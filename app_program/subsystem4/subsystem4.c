#include "subsystem4.h"
#include "drv_gy511.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../common/robot_snapshot.h"
#include "../subsystem1/subsystem1.h"
#include "../subsystem2/drv_encoder.h"
#include "../subsystem2/subsystem2.h"

static rd_word_t terrain_word, startup_outcome; /* writer: I/O */
static ID snapshot_mutex;
static S4Snapshot snapshot; /* guarded by snapshot_mutex */
static TerrainContext context;

uint32_t s4_terrain_word(void)
{
    return rd_word_read(&terrain_word);
}

bool s4_startup_complete(void)
{
    return rd_word_read(&startup_outcome) != 0u;
}

bool s4_copy(S4Snapshot *out)
{
    if (out == NULL || snapshot_mutex <= 0 ||
        tk_loc_mtx(snapshot_mutex, RD_SNAPSHOT_LOCK_MS) < E_OK)
    {
        return false;
    }
    *out = snapshot;
    return tk_unl_mtx(snapshot_mutex) >= E_OK;
}

static void publish(uint32_t now, gy511_sample_result_t result)
{
    static S4Snapshot latest; /* only I/O writes, even if a copy is skipped */
    latest.terrain = terrain_pack(context.last_hump_code, context.max_hump_code,
        context.hump_count, (uint32_t)context.state, context.height_valid,
        context.is_calibrated);
    latest.serviced_ms = now;
    ++latest.sequence;
    if (result == GY511_SAMPLE_ERROR)
    {
        latest.acquisition_valid = 0u;
    }
    if (result == GY511_SAMPLE_FRESH)
    {
        latest.sampled_ms = now;
        latest.acquisition_valid = 1u;
    }
    rd_word_publish(&terrain_word, latest.terrain);
    if (tk_loc_mtx(snapshot_mutex, TMO_POL) >= E_OK)
    {
        snapshot = latest;
        (void)tk_unl_mtx(snapshot_mutex);
    }
}

void s4_step(uint32_t now)
{
    static uint8_t initialized, hardware_ready;
    static uint32_t started_ms, next_sample_ms, acquired_ms;
    static uint8_t have_acquisition;
    static uint32_t last_left, last_right;
    static uint8_t previous_humps;
    if (!initialized)
    {
        terrain_algo_init(&context);
        started_ms = now;
        next_sample_ms = now;
        last_left = drv_encoder_count(ENCODER_LEFT);
        last_right = drv_encoder_count(ENCODER_RIGHT);
        hardware_ready = drv_gy511_init() ? 1u : 0u;
        initialized = 1u;
        if (!hardware_ready)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU | RD_FAULT_CALIBRATION_FAILED);
            rd_word_publish(&startup_outcome, 2u);
        }
        publish(robot_time_ms(), GY511_SAMPLE_ERROR);
        return;
    }
    if ((int32_t)(now - next_sample_ms) < 0)
    {
        return;
    }
    bool calibrating = !s4_startup_complete();
    next_sample_ms = now + (calibrating ? TERRAIN_CALIBRATION_PERIOD_MS : TERRAIN_PERIOD_MS);
    uint32_t left = drv_encoder_count(ENCODER_LEFT);
    uint32_t right = drv_encoder_count(ENCODER_RIGHT);
    bool stationary = left == last_left && right == last_right;
    uint32_t mean = enc_mean(left, right);
    int32_t moved = enc_travel_signed(mean, enc_mean(last_left, last_right));
    last_left = left;
    last_right = right;
    Gy511RawAxes raw = {0};
    gy511_sample_result_t result = hardware_ready ? drv_gy511_sample(&raw) : GY511_SAMPLE_ERROR;
    now = robot_time_ms();
    bool acquired = result == GY511_SAMPLE_FRESH;
    if (result == GY511_SAMPLE_ERROR)
    {
        rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU);
    }
    else if (acquired)
    {
        acquired_ms = now;
        have_acquisition = 1u;
        rd_fault_clear(RD_OWNER_TERRAIN, RD_FAULT_IMU);
    }
    if (!acquired && (!have_acquisition ||
        robot_time_elapsed(now, acquired_ms) > SAFETY_STALE_MS))
    {
        rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU);
        terrain_algo_invalidate_window(&context);
    }
    if (calibrating)
    {
        if (!stationary)
        {
            terrain_algo_calibration_restart(&context);
        }
        else if (acquired)
        {
            (void)terrain_algo_calibrate_sample(&context, &raw);
        }
        if (context.is_calibrated)
        {
            rd_word_publish(&startup_outcome, 1u);
        }
        else if (robot_time_elapsed(now, started_ms) >= TERRAIN_CALIBRATION_TIMEOUT_MS)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_CALIBRATION_FAILED);
            rd_word_publish(&startup_outcome, 2u);
        }
    }
    else if (acquired && context.is_calibrated)
    {
        MotionHealth health = {0};
        bool health_copied = s2_health_try(&health) >= E_OK;
        bool encoder_valid = health_copied && health.sequence != 0u &&
            robot_time_elapsed(now, health.serviced_ms) <= SAFETY_STALE_MS &&
            (rd_fault_get(RD_OWNER_MOTION) & RD_FAULT_ENCODER) == 0u;
        if (!encoder_valid)
        {
            /* Missing travel invalidates this window; keep completed results. */
            terrain_algo_invalidate_window(&context);
        }
        terrain_algo_process(&context, &raw, encoder_valid && moved > 0 ? (uint32_t)moved : 0u);
        if (!encoder_valid)
        {
            terrain_algo_invalidate_window(&context);
        }
        bool impact = terrain_impact_pending(
            (rd_fault_get(RD_OWNER_TERRAIN) & RD_FAULT_IMPACT) != 0u,
            context.state == TERRAIN_STATE_IMPACT,
            health_copied && motion_health_brake_acknowledged(&health));
        if (impact)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMPACT);
        }
        else
        {
            rd_fault_clear(RD_OWNER_TERRAIN, RD_FAULT_IMPACT);
        }
        if (context.hump_count != previous_humps)
        {
            previous_humps = context.hump_count;
            s1_event(MQTT_EVENT_HUMP, context.last_hump_code, 0u);
        }
    }
    if (result == GY511_SAMPLE_ERROR)
    {
        terrain_algo_invalidate_window(&context);
    }
    publish(now, result);
}

ER subsystem4_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    snapshot_mutex = tk_cre_mtx(&mutex);
    return snapshot_mutex > 0 ? E_OK : E_SYS;
}
