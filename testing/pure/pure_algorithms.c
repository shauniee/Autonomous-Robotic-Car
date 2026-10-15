#define PROHIBIT_DEF_SIZE_T
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../fixture_config.h"
#include "../subsystem3/code39_vectors.h"
#include "../../app_program/subsystem1/mapping_protocol.h"
#include "../../app_program/subsystem1/telemetry.h"
#include "../../app_program/subsystem2/motion_control.h"
#include "../../app_program/subsystem3/barcode_decode.h"
#include "../../app_program/subsystem3/ir_tracking.h"
#include "../../app_program/subsystem4/terrain_algo.h"
#include "../../app_program/subsystem5/subsystem5.h"
#include "../../app_program/subsystem3/mission_guards.h"

static unsigned failures;

static void report(const char *name, bool pass)
{
    printf("[PURE] %s %s\n", name, pass ? "PASS" : "FAIL");
    failures += pass ? 0u : 1u;
}

static bool mapping_parser(void)
{
    static S1MappingRequest request;
    static char payload[512];
    strcpy(payload,
           "{\"request\":\"barcode_mapping\",\"request_id\":4294967295,\"mapping\":[");
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        strcat(payload, i == 0u ? "\"unmapped\"" : ",\"unmapped\"");
    }
    strcat(payload, "]}");
    unsigned length = (unsigned)strlen(payload);
    bool valid = s1_mapping_begin(&request, length) &&
                 s1_mapping_data(&request, payload, 19u, false) &&
                 s1_mapping_data(&request, payload + 19u, length - 19u, true) &&
                 request.request_id == UINT32_MAX &&
                 !s1_mapping_begin(&request, 513u);
    static const char bad[] =
        "{\"request\":\"barcode_mapping\",\"request_id\":01,\"mapping\":[]}";
    length = sizeof(bad) - 1u;
    return valid && s1_mapping_begin(&request, length) &&
           !s1_mapping_data(&request, bad, length, true);
}

static bool telemetry_freshness(void)
{
    S1Telemetry status = {.sampled_ms = UINT32_MAX - 39u,
                          .reading_valid = true,
                          .motion_ms = UINT32_MAX - 39u,
                          .motion_valid = true,
                          .terrain_ms = UINT32_MAX - 229u,
                          .terrain_valid = true,
                          .magnetic_ms = UINT32_MAX - 229u,
                          .magnetic_valid = true,
                          .front_ms = UINT32_MAX - 229u,
                          .front_valid = true,
                          .front_mm = 234,
                          .left_mm = 12,
                          .right_mm = 34,
                          .accel_mg = {0, 0, 1000},
                          .magnetic_milli_gauss = {1, 2, 3}};
    static char json[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    unsigned length = s1_telemetry_encode(&status, 20u, json, sizeof(json));
    return length > 0u && strstr(json, "\"distance_mm\":[12,34]") &&
           strstr(json, "\"accel_mg\":[0,0,1000]") &&
           strstr(json, "\"mag_milli_gauss\":[1,2,3]") &&
           strstr(json, "\"front_mm\":234");
}

static bool motion_completion(void)
{
    RobotConfig config = fixture_config();
    MotionController control;
    MotionCommand command = {.action_id = 1u,
                             .run_epoch = 7u,
                             .mode = MOTION_DISTANCE,
                             .target = 0.1f,
                             .speed_mps = 0.1f,
                             .sampled_ms = 100u,
                             .issued_ms = 100u,
                             .expires_ms = 160u,
                             .deadline_ms = 1100u};
    motion_control_init(&control, 7u);
    bool pass = motion_control_accept(&control, &command, 0.0f, 0.0f,
                                      100u, &config);
    command.sampled_ms = command.issued_ms = 120u;
    command.expires_ms = 180u;
    pass = pass && motion_control_accept(&control, &command, 0.04f, 0.04f,
                                         120u, &config) &&
           control.origin_left == 0.0f &&
           control.command.deadline_ms == 1100u;
    float left, right;
    motion_control_step(&control, 0.1f, 0.05f, 0.0f, 0.0f, 0.02f, 139u,
                        &config, &left, &right);
    pass = pass && control.status == MOTION_ACTIVE && left == 0.0f &&
           right > 0.0f;
    motion_control_step(&control, 0.1f, 0.1f, 0.0f, 0.0f, 0.02f, 140u,
                        &config, &left, &right);
    return pass && control.status == MOTION_DONE && left == 0.0f &&
           right == 0.0f && motion_control_ack(&control, 1u);
}

static void alphabet_frame(unsigned letter, unsigned scale, bool reverse,
                           uint32_t widths[BARCODE_ELEMENTS])
{
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        widths[i] = 1000u * scale;
    }
    const unsigned star[] = {1u, 4u, 6u};
    for (unsigned i = 0u; i < 3u; ++i)
    {
        widths[star[i]] *= 3u;
        widths[20u + star[i]] *= 3u;
        widths[10u + code39_wide[letter][i]] *= 3u;
    }
    if (reverse)
    {
        for (unsigned i = 0u; i < 14u; ++i)
        {
            uint32_t value = widths[i];
            widths[i] = widths[28u - i];
            widths[28u - i] = value;
        }
    }
}

static bool barcode_alphabet(void)
{
    RobotConfig config = fixture_config();
    uint32_t widths[BARCODE_ELEMENTS];
    for (unsigned letter = 0u; letter < BARCODE_ALPHABET; ++letter)
    {
        for (unsigned scale = 1u; scale <= 2u; ++scale)
        {
            for (unsigned reverse = 0u; reverse < 2u; ++reverse)
            {
                alphabet_frame(letter, scale, reverse != 0u, widths);
                char symbol = 0;
                barcode_orientation_t orientation;
                if (barcode_decode(widths, &config.barcode, &symbol,
                                   &orientation) != BARCODE_DECODED ||
                    symbol != (char)('A' + letter) ||
                    orientation != (reverse ? BARCODE_REVERSE : BARCODE_FORWARD))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool ir_capture_guidance(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    bool pass = ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u);
    for (uint32_t now = 0u; now <= 500u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){500u, 500u}, true, now);
    }
    pass = pass && tracker.calibration.black_set &&
           ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 520u);
    for (uint32_t now = 520u; now <= 1020u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){3000u, 3000u}, true, now);
    }
    ir_tracking_sample(&tracker, (IrRaw){500u, 3000u}, true, 1040u);
    IrTrackingOutput output = ir_tracking_output(&tracker, 1040u);
    return pass && tracker.calibration.white_set && output.valid &&
           output.steering < 0.0f &&
           !ir_tracking_output(&tracker, 1101u).valid;
}

static bool terrain_calibration(void)
{
    TerrainContext context;
    terrain_algo_init(&context);
    const Gy511RawAxes level = {0, 0, 1000};
    for (unsigned i = 0u; i < 100u; ++i)
    {
        (void)terrain_algo_calibrate_sample(&context, &level);
    }
    return context.is_calibrated && context.calib_count == 100u &&
           fabsf(context.accel_offset.z) < 0.001f;
}

static bool obstacle_profile(void)
{
    RobotConfig config = fixture_config();
    ObstacleSnapshot snapshot = {.ray_count = 5u};
    for (unsigned i = 0u; i < 5u; ++i)
    {
        snapshot.rays[i] = (ObstacleRay){(float)((int)i - 2) * 30.0f,
                                         i == 2u ? 0.2f : 1.0f, 100u, true};
    }
    return s5_profile_build_config(&snapshot, &config) &&
           snapshot.closest_m > 0.21f && snapshot.obstacle_forward_m > 0.21f;
}

int main(void)
{
    report("S1-MAPPING", mapping_parser());
    report("S1-TELEMETRY", telemetry_freshness());
    report("S2-MOTION", motion_completion());
    report("S3-BARCODE-AZ", barcode_alphabet());
    report("S3-IR-CAPTURE", ir_capture_guidance());
    report("S4-TERRAIN-CAL", terrain_calibration());
    report("S5-PROFILE", obstacle_profile());
    return failures == 0u ? 0 : 1;
}
