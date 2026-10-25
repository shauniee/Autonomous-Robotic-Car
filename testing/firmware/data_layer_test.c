#include "../../app_program/app.h"
#include "../../app_program/common/robot_data.h"
#include "test_report.h"

#if APP_TEST
/* INT-12: shared-data layer on the target. Every check has a pass criterion
   that can fail; it exercises accessors at minimum, maximum and invalid codes
   and the fault-byte ownership rule. It does not test cross-core behaviour. */
void data_layer_test(void)
{
    TEST_INIT();
    bool ok;
    uint32_t w = rd_field_set(0u, 3u, 5u, 31u, &ok);
    TEST_CHECK("INT-12", ok && rd_field_get(w, 3u, 5u) == 31u, "field maximum stored and read back");
    uint32_t same = rd_field_set(w, 3u, 5u, 32u, &ok);
    TEST_CHECK("INT-12", !ok && same == w, "field value one past the maximum is rejected, word unchanged");
    TEST_CHECK("INT-12", rd_sat_valid(1000u, 7u) == 126u && rd_invalid_code(7u) == 127u,
               "7-bit result saturates below the invalid code");

    const uint8_t letters[RD_MAP_COUNT] = {22u, 5u, 0u, 25u};
    uint32_t map = 0u;
    rd_map_action_t action;
    TEST_CHECK("INT-12", rd_mapping_pack(letters, &map) && rd_mapping_lookup(map, 25u, &action) &&
               action == RD_MAP_UTURN, "mapping packs four letters and looks one up");
    const uint8_t dup[RD_MAP_COUNT] = {1u, 2u, 2u, 3u};
    uint32_t keep = map;
    TEST_CHECK("INT-12", !rd_mapping_pack(dup, &map) && map == keep, "duplicate letters rejected, word unchanged");
    TEST_CHECK("INT-12", !rd_mapping_lookup(0u, 0u, &action), "boot mapping word is unmapped");

    rd_fault_set(RD_OWNER_NETWORK, 0x3u);
    bool network_only = !rd_fault_stop_pending();
    rd_fault_set(RD_OWNER_MOTION, 0x1u);
    bool motion_stops = rd_fault_stop_pending();
    rd_fault_clear(RD_OWNER_MOTION, 0x1u);
    rd_fault_clear(RD_OWNER_NETWORK, 0x3u);
    TEST_CHECK("INT-12", network_only && motion_stops && rd_fault_aggregate() == 0u,
               "network bits never stop motion; an owner bit does; bits clear");
    TEST_SUMMARY("data_layer");
}
#endif
