#include "../app.h"
#include "comms.h"
#include "../test_report.h"
#if APP_TEST
void comms_test(void)
{
    /* On-target check of the frame codec the host tests also cover (S1-U09). */
    uint8_t frame[MQTT_FRAME_MAX];
    mqtt_telemetry_t t = {.sequence = 1, .mission_state = 2, .front_mm = 1000, .front_valid = true,
                          .barcode_letter = MQTT_NO_LETTER};
    mqtt_telemetry_t back;
    mqtt_type_t type;
    bool pass = mqtt_encode_telemetry(frame, sizeof frame, MQTT_TYPE_TELEMETRY, &t) == MQTT_LEN_TELEMETRY &&
                mqtt_decode_telemetry(frame, MQTT_LEN_TELEMETRY, &type, &back) == MQTT_DECODE_OK &&
                back.front_mm == 1000u && !(mqtt_decode_telemetry(frame, MQTT_LEN_TELEMETRY - 1u, &type, &back) == MQTT_DECODE_OK);
    TEST_INIT();
    TEST_CHECK("S1-U09", pass, "binary telemetry round-trip and length check");
    TEST_SUMMARY("comms");
}
#endif
