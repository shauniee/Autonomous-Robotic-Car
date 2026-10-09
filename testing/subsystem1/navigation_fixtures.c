#include "navigation_fixtures.h"
#include "../../app_program/subsystem1/telemetry.h"
#include <string.h>
#include <stdint.h>
bool s1_navigation_fixture_encode(void)
{
    S1Telemetry snapshot = {.sequence = UINT32_MAX,
        .motion_available = true, .motion_serviced_ms = 0u,
        .applied_left = -350, .applied_right = 350,
        .encoder_valid = true, .encoder_calibrated = true,
        .encoder_left_count = UINT32_MAX, .encoder_right_count = UINT32_MAX,
        .encoder_errors = UINT32_MAX,
        .speed_left_milli = -2000000000, .speed_right_milli = 2000000000,
        .target_left_milli = -2000000000, .target_right_milli = 2000000000,
        .velocity_mode = true, .speed_target_valid = true,
        .action_id = UINT32_MAX, .action_state = 3u,
        .barcode_available = true, .barcode_sequence = UINT32_MAX,
        .barcode_queued = UINT32_MAX,
        .barcode_accepted = UINT32_MAX, .barcode_rejected = UINT32_MAX,
        .barcode_discarded = UINT32_MAX, .barcode_state = "fault",
        .barcode_result = "capture_overflow", .barcode_symbol = "Z"};
    /* Static avoids putting two payload buffers on the startup stack. */
    static char json[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    unsigned length = s1_telemetry_encode(&snapshot, 60u, json, sizeof(json));
    if ((length == 0u) || (strstr(json, "capture_overflow") == NULL) ||
        (strstr(json, "-2000000.000") == NULL) ||
        (strstr(json, "\"symbol\":\"Z\"") == NULL))
    {
        return false;
    }
    if (s1_telemetry_encode(&snapshot, 60u, json, length + 1u) != length ||
        s1_telemetry_encode(&snapshot, 60u, json, length) != 0u || json[0])
    {
        return false;
    }
    snapshot.barcode_available = false;
    snapshot.motion_available = false;
    snapshot.speed_target_valid = false;
    length = s1_telemetry_encode(&snapshot, 61u, json, sizeof(json));
    return (length != 0u) && (strstr(json, "\"barcode\":null") != NULL) &&
        (strstr(json, "\"encoders\":null") != NULL) &&
        (strstr(json, "\"requested_speed_mps\":null") != NULL);
}
