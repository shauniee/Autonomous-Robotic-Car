#include "../app.h"
#include "terrain.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../comms/comms.h"
#include "../motion/motion.h"
#include "../../device/i2c/sysdepend/rp2040/i2c_sysdep.h"

/* ---- Terrain task ---- */
static rd_word_t terrain_word, startup_outcome; /* writer: I/O */
static TerrainContext context;

uint32_t terrain_read_word(void)
{
    return rd_word_read(&terrain_word);
}

bool terrain_startup_complete(void)
{
    return rd_word_read(&startup_outcome) != 0u;
}

/* INT-154 diagnostics: why an encounter did or did not count. Same task as the
   reader, so one pending frame is enough: every sample during an encounter, and
   one per second on flat ground. */
static uint8_t trace_frame[MQTT_LEN_TERRAIN_TRACE];
static bool trace_pending;
static uint32_t trace_last_ms;
static float trace_travel_mm;

/* Plain C: <math.h> clashes with the kernel's size_t in this file. */
static int8_t trace_clamp(float value)
{
    float limited = (value > 127.0f) ? 127.0f :
        ((value < -127.0f) ? -127.0f : value);

    /* NOTE: limited above, so the cast cannot overflow. */
    return (int8_t)(limited + ((limited >= 0.0f) ? 0.5f : -0.5f));
}

static uint8_t trace_byte(float value)
{
    float limited = (value > 255.0f) ? 255.0f :
        ((value < 0.0f) ? 0.0f : value);

    /* NOTE: limited to 0..255, so the cast cannot overflow. */
    return (uint8_t)(limited + 0.5f);
}

static void terrain_trace_note(uint32_t now, const Gy511RawAxes *p_raw,
    bool b_encoder_valid, float moved_counts)
{
    const float mg_per_lsb = ACCEL_MG_PER_LSB * 1000.0f;
    uint32_t period_ms = (TERRAIN_STATE_FLAT == context.state) ?
        TERRAIN_TRACE_FLAT_MS : TERRAIN_TRACE_HUMP_MS;

    trace_travel_mm += moved_counts / ENC_COUNTS_PER_MM;
    if (trace_travel_mm >= 65536.0f)
    {
        trace_travel_mm -= 65536.0f;
    }
    if (robot_time_elapsed(now, trace_last_ms) >= period_ms)
    {
        mqtt_terrain_trace_t trace =
        {
            /* NOTE: the low 16 bits are the wire format's rolling time. */
            .time_ms = (uint16_t)now,
            .pitch_q2 = trace_clamp((float)context.pitch_cdeg / 25.0f),
            /* NOTE: state is 3 bits, so the flags stay within a byte. */
            .state_flags = (uint8_t)(context.state |
                (b_encoder_valid ? 0x40u : 0u) |
                (context.is_calibrated ? 0x80u : 0u)),
            .rise_mm = trace_byte((float)context.rise_q / 10.0f),
            .accel_10mg = trace_byte(terrain_acceleration_mg(p_raw) / 10.0f),
            .roll_q2 = trace_clamp((float)context.roll_cdeg / 25.0f),
            .accel_16mg =
            {
                trace_clamp((float)p_raw->x * mg_per_lsb / 16.0f),
                trace_clamp((float)p_raw->y * mg_per_lsb / 16.0f),
                trace_clamp((float)p_raw->z * mg_per_lsb / 16.0f)
            },
            .height_mm = trace_byte(terrain_height_mm(&context)),
            /* NOTE: kept below 65536 above, so the cast cannot overflow. */
            .travel_mm = (uint16_t)trace_travel_mm
        };

        trace_last_ms = now;
        (void)mqtt_encode_terrain_trace(trace_frame, sizeof trace_frame,
            &trace);
        trace_pending = true;
    }
}

unsigned terrain_trace_take(uint8_t *out, unsigned capacity)
{
    if (!trace_pending || out == NULL || capacity < MQTT_LEN_TERRAIN_TRACE)
    {
        return 0u;
    }
    trace_pending = false;
    for (unsigned i = 0u; i < MQTT_LEN_TERRAIN_TRACE; ++i)
    {
        out[i] = trace_frame[i];
    }
    return MQTT_LEN_TERRAIN_TRACE;
}

static void publish(void)
{
    rd_word_publish(&terrain_word, terrain_pack(context.last_hump_code,
        context.max_hump_code, context.hump_count, (uint32_t)context.state,
        context.height_valid, context.is_calibrated));
}

void terrain_step(uint32_t now)
{
    static uint8_t initialized, hardware_ready;
    static uint32_t started_ms, next_sample_ms, acquired_ms;
    static uint8_t have_acquisition;
    static enc_count_t last_left, last_right;
    static enc_count_t travel_origin[2];
    static uint8_t previous_humps;
    if (!initialized)
    {
        terrain_algo_init(&context);
        started_ms = now;
        next_sample_ms = now;
        last_left = drv_encoder_count(ENCODER_LEFT);
        last_right = drv_encoder_count(ENCODER_RIGHT);
        travel_origin[0] = last_left;
        travel_origin[1] = last_right;
        hardware_ready = drv_gy511_init() ? 1u : 0u;
        initialized = 1u;
        if (!hardware_ready)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU | RD_FAULT_CALIBRATION_FAILED);
            rd_word_publish(&startup_outcome, 2u);
            tm_printf((UB *)"[IMU] calibration failed: I2C init failed\n");
        }
        else
        {
            tm_printf((UB *)"[IMU] calibration started: keep wheels still; "
                "timeout_ms=%u\n", (unsigned)TERRAIN_CALIBRATION_TIMEOUT_MS);
        }
        publish();
        return;
    }
    if ((int32_t)(now - next_sample_ms) < 0)
    {
        return;
    }
    bool calibrating = !terrain_startup_complete();
    next_sample_ms = now + (calibrating ? TERRAIN_CALIBRATION_PERIOD_MS : TERRAIN_PERIOD_MS);
    uint32_t left = drv_encoder_count(ENCODER_LEFT);
    uint32_t right = drv_encoder_count(ENCODER_RIGHT);
    bool stationary = left == last_left && right == last_right;
    last_left = left;
    last_right = right;
    Gy511RawAxes raw = {0};
    gy511_sample_result_t result = hardware_ready ? drv_gy511_sample(&raw) : GY511_SAMPLE_ERROR;
    now = robot_time_ms();
    bool acquired = result == GY511_SAMPLE_FRESH;
#if TERRAIN_DIAGNOSTICS
    bool trace_encoder_valid = false;
#endif
    bool stale = !have_acquisition ||
        robot_time_elapsed(now, acquired_ms) > SAFETY_STALE_MS;
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
#if TERRAIN_DIAGNOSTICS
    const int32_t delta[2] = {
        enc_travel_signed(left, travel_origin[0]),
        enc_travel_signed(right, travel_origin[1])};
#endif
    const enc_count_t count[2] = {(enc_count_t)left, (enc_count_t)right};
    float moved = terrain_travel_take(travel_origin, count, acquired,
        calibrating || result == GY511_SAMPLE_ERROR || stale);
    if (stale)
    {
        if (!acquired)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU);
        }
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
            tm_printf((UB *)"[IMU] calibration passed: samples=%u elapsed_ms=%u\n",
                (unsigned)context.calib_count,
                (unsigned)robot_time_elapsed(now, started_ms));
        }
        else if (robot_time_elapsed(now, started_ms) >= TERRAIN_CALIBRATION_TIMEOUT_MS)
        {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_CALIBRATION_FAILED);
            rd_word_publish(&startup_outcome, 2u);
            tm_printf((UB *)"[IMU] calibration failed: timeout samples=%u/100 "
                "elapsed_ms=%u stationary=%u sample=%s\n",
                (unsigned)context.calib_count,
                (unsigned)robot_time_elapsed(now, started_ms),
                (unsigned)stationary,
                acquired ? "fresh" :
                (result == GY511_SAMPLE_PENDING ? "pending" : "I2C-error"));
        }
    }
    else if (acquired && context.is_calibrated)
    {
        MotionHealth health = {0};
        bool health_copied = motion_health_try(&health) >= E_OK;
        /* A lock this poll lost says nothing about the encoders (INT-157). */
        bool encoder_valid = (!health_copied || (health.sequence != 0u &&
            robot_time_elapsed(now, health.serviced_ms) <= SAFETY_STALE_MS)) &&
            (rd_fault_get(RD_OWNER_MOTION) & RD_FAULT_ENCODER) == 0u;
        if (!encoder_valid)
        {
            /* Missing travel invalidates this window; keep completed results. */
            terrain_algo_invalidate_window(&context);
        }
#if TERRAIN_DIAGNOSTICS
        trace_encoder_valid = encoder_valid;
#endif
        terrain_algo_process(&context, &raw, encoder_valid && moved > 0.0f ? moved : 0.0f, stationary);
        terrain_trace_note(now, &raw, encoder_valid,
            (encoder_valid && (moved > 0.0f)) ? moved : 0.0f);
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
            comms_event(MQTT_EVENT_HUMP, context.last_hump_code, 0u);
        }
    }
    if (result == GY511_SAMPLE_ERROR)
    {
        terrain_algo_invalidate_window(&context);
    }
    publish();
#if TERRAIN_DIAGNOSTICS
    /* USB queues without waiting for a host; losses remain visible in the trace. */
    if (acquired)
    {
        static uint32_t trace_sequence;
        extern volatile UW tm_usb_dropped;
        tm_printf((UB *)"[IMU_RAW] %u,%u,%d,%d,%d,%d,%d,%d,"
            "%u,%u,%u,%d,%u,%u,%u,%u,%u\n",
            (unsigned)++trace_sequence, (unsigned)now,
            (int)raw.x, (int)raw.y, (int)raw.z,
            (int)context.pitch_cdeg, (int)delta[0], (int)delta[1],
            (unsigned)context.is_calibrated, (unsigned)context.state,
            (unsigned)trace_encoder_valid, (int)context.roll_cdeg,
            (unsigned)stationary,
            (unsigned)context.height_valid, (unsigned)context.hump_count,
            (unsigned)context.rise_q, (unsigned)tm_usb_dropped);
    }
#endif
}

ER terrain_init(void)
{
    return E_OK;
}

/* ---- GY-511 driver ---- */
/*
 * terrain.c - Bounded I2C0 implementation for GY-511 (LSM303DLHC).
 */

/* Registers for LSM303DLHC */
#define REG_CTRL_REG1_A 0x20u
#define REG_CTRL_REG4_A 0x23u
/* ST LSM303DLHC datasheet section 7.1.8: STATUS_REG_A, ZYXDA.
   https://www.st.com/resource/en/datasheet/lsm303dlhc.pdf */
#define REG_STATUS_A 0x27u
#define STATUS_NEW_XYZ 0x08u
#define REG_OUT_X_L_A 0x28u
#define REG_AUTO_INC_A 0x80u

static uint32_t operation_started;

/* Wait with timeout for I2C status flag */
static bool wait_i2c_status(uint32_t mask, bool match_set, uint32_t timeout_us)
{
    uint32_t start = robot_time_us();
    while (true)
    {
        if ((in_w(I2C0_BASE + I2Cx_RAW_INTR_STAT) & I2C_INT_TX_ABRT) != 0u)
        {
            (void)in_w(I2C0_BASE + I2Cx_CLR_TX_ABRT);
            out_w(I2C0_BASE + I2Cx_ENABLE, 0u);
            return false;
        }
        if (robot_time_elapsed(robot_time_us(), operation_started) >= GY511_TIMEOUT_US)
        {
            out_w(I2C0_BASE + I2Cx_ENABLE, 0u);
            return false;
        }
        uint32_t status = in_w(I2C0_BASE + I2Cx_STATUS);
        bool condition = (status & mask) != 0;
        if (condition == match_set)
        {
            return true;
        }
        if ((uint32_t)(robot_time_us() - start) >= timeout_us ||
            robot_time_elapsed(robot_time_us(), operation_started) >= GY511_TIMEOUT_US)
        {
            return false;
        }
    }
}

static bool wait_stop(void)
{
    while ((in_w(I2C0_BASE + I2Cx_RAW_INTR_STAT) & I2C_INT_STOP_DET) == 0u)
    {
        if ((in_w(I2C0_BASE + I2Cx_RAW_INTR_STAT) & I2C_INT_TX_ABRT) != 0u ||
            robot_time_elapsed(robot_time_us(), operation_started) >= GY511_TIMEOUT_US)
        {
            (void)in_w(I2C0_BASE + I2Cx_CLR_TX_ABRT);
            out_w(I2C0_BASE + I2Cx_ENABLE, 0u);
            return false;
        }
    }
    (void)in_w(I2C0_BASE + I2Cx_CLR_STOP_DET);
    return true;
}

/* Transmit 1 byte with timeout */
static bool i2c0_write_byte(uint8_t byte, bool stop_bit)
{
    if (!wait_i2c_status(I2C_STATUS_TFNF, true, GY511_TIMEOUT_US))
    {
        return false;
    }
    uint32_t cmd = byte | (stop_bit ? I2C_DATA_CMD_STOP : 0);
    out_w(I2C0_BASE + I2Cx_DATA_CMD, cmd);
    return true;
}

/* Write single register to slave */
static bool i2c0_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val)
{
    out_w(I2C0_BASE + I2Cx_ENABLE, 0); /* Disable to set target */
    out_w(I2C0_BASE + I2Cx_TAR, slave_addr);
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);
    out_w(I2C0_BASE + I2Cx_ENABLE, 1); /* Enable */

    if (!i2c0_write_byte(reg, false))
        return false;
    if (!i2c0_write_byte(val, true))
        return false;

    /* Wait for transmission to finish */
    return wait_stop();
}

/* Read multi-byte buffer from slave */
static bool i2c0_read_buffer(uint8_t slave_addr, uint8_t reg, uint8_t *buf,
                             uint8_t len)
{
    out_w(I2C0_BASE + I2Cx_ENABLE, 0);
    out_w(I2C0_BASE + I2Cx_TAR, slave_addr);
    (void)in_w(I2C0_BASE + I2Cx_CLR_INTR);
    out_w(I2C0_BASE + I2Cx_ENABLE, 1);

    /* Send register sub-address */
    if (!i2c0_write_byte(reg, false))
        return false;

    /* Issue read commands into FIFO */
    for (uint8_t i = 0; i < len; ++i)
    {
        bool is_last = (i == (len - 1));
        uint32_t cmd =
            I2C_DATA_CMD_CMD_READ | (is_last ? I2C_DATA_CMD_STOP : 0);
        if (!wait_i2c_status(I2C_STATUS_TFNF, true, GY511_TIMEOUT_US))
        {
            return false;
        }
        out_w(I2C0_BASE + I2Cx_DATA_CMD, cmd);
    }

    /* Collect returned bytes */
    for (uint8_t i = 0; i < len; ++i)
    {
        if (!wait_i2c_status(I2C_STATUS_RFNE, true, GY511_TIMEOUT_US))
        {
            return false;
        }
        buf[i] = (uint8_t)(in_w(I2C0_BASE + I2Cx_DATA_CMD) & 0xFF);
    }

    return wait_stop();
}

bool drv_gy511_init(void)
{
    operation_started = robot_time_us();
    /* 1. Release I2C0 and IO_BANK0 resets */
    clr_w(RESETS_RESET, RESETS_RESET_I2C0);
    uint32_t start = robot_time_us();
    while (!(in_w(RESETS_RESET_DONE) & RESETS_RESET_I2C0))
    {
        if ((uint32_t)(robot_time_us() - start) >= GY511_TIMEOUT_US)
            return false;
    }

    /* 2. Configure GP4 (SDA) and GP5 (SCL) mux to I2C (func 3) with pull-ups */
    for (unsigned p = 4; p <= 5; ++p)
    {
        out_w(GPIO_CTRL(p), GPIO_CTRL_FUNCSEL_I2C);
        clr_w(GPIO(p), GPIO_OD);
        set_w(GPIO(p), GPIO_IE | GPIO_PUE);
    }

    /* 3. Configure Synopsys I2C0 controller (400 kHz Fast Mode at 125 MHz
     * sys_clk) */
    out_w(I2C0_BASE + I2Cx_ENABLE, 0);
    out_w(I2C0_BASE + I2Cx_CON, I2C_CON_MASTER_MODE | I2C_CON_SPEED_FAST |
                                    I2C_CON_RESTART_EN | I2C_CON_SLAVE_DISABLE |
                                    I2C_CON_TX_EMPTY_CTRL);

    /* Conservative fast-mode counts below 400 kHz at template 125 MHz.
       Verify actual rise/fall timing and pull-ups on the board. */
    out_w(I2C0_BASE + I2Cx_FS_SCL_HCNT, 150);
    out_w(I2C0_BASE + I2Cx_FS_SCL_LCNT, 200);

    out_w(I2C0_BASE + I2Cx_ENABLE, 1);

    /* 4. Configure accelerometer: 50 Hz, all axes, +/- 4 g high resolution
       (CTRL_REG4_A: BDU, FS = 01, HR), so a 2.5 g impact is measurable. */
    if (!i2c0_write_reg(GY511_I2C_ADDR_ACCEL, REG_CTRL_REG1_A, 0x47u))
        return false;
    if (!i2c0_write_reg(GY511_I2C_ADDR_ACCEL, REG_CTRL_REG4_A, 0x98u))
        return false;

    return true;
}

gy511_sample_result_t drv_gy511_sample(Gy511RawAxes *out_raw)
{
    operation_started = robot_time_us();
    if (out_raw == NULL)
    {
        return GY511_SAMPLE_ERROR;
    }
    uint8_t status = 0u;
    if (!i2c0_read_buffer(GY511_I2C_ADDR_ACCEL, REG_STATUS_A, &status, 1u))
    {
        return GY511_SAMPLE_ERROR;
    }
    if ((status & STATUS_NEW_XYZ) == 0u)
    {
        return GY511_SAMPLE_PENDING; /* unchanged registers are not a new acquisition */
    }
    uint8_t buf[6] = {0};
    /* Auto-increment bit 7 required on LSM303 */
    if (!i2c0_read_buffer(GY511_I2C_ADDR_ACCEL, REG_OUT_X_L_A | REG_AUTO_INC_A,
                          buf, 6))
    {
        return GY511_SAMPLE_ERROR;
    }
    out_raw->x = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
    out_raw->y = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
    out_raw->z = (int16_t)((buf[5] << 8) | buf[4]) >> 4;
    return GY511_SAMPLE_FRESH;
}
