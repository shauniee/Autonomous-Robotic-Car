#include "../../app_program/common/robot_config.h"
#include "../../app_program/subsystem3/mission.h"
#include "../../app_program/subsystem3/mission_guards.h"
#include "../fixture_config.h"
#include "../../app_program/test_report.h"
#if APP_TEST
static bool polling_mutex_test(void)
{
    T_RTSK before = {0}, held = {0}, released = {0};
    if (tk_ref_tsk(TSK_SELF, &before) < E_OK)
    {
        return false;
    }
    T_CMTX definition = {.mtxatr = TA_CEILING,
                         .ceilpri = APP_SHARED_MUTEX_CEILING};
    ID id = tk_cre_mtx(&definition);
    if (id <= 0)
    {
        return false;
    }
    bool changed = tk_chg_pri(TSK_SELF, 20) >= E_OK;
    bool locked = changed && tk_loc_mtx(id, TMO_POL) >= E_OK;
    bool protected = locked && tk_ref_tsk(TSK_SELF, &held) >= E_OK &&
                     held.tskpri == APP_SHARED_MUTEX_CEILING &&
                     held.tskbpri == 20;
    ER unlocked = locked ? tk_unl_mtx(id) : E_OBJ;
    bool lowered = unlocked >= E_OK &&
                   tk_ref_tsk(TSK_SELF, &released) >= E_OK &&
                   released.tskpri == 20 && released.tskbpri == 20;
    ER restored = tk_chg_pri(TSK_SELF, before.tskbpri);
    ER deleted = tk_del_mtx(id);
    return protected && lowered && restored >= E_OK && deleted >= E_OK;
}

static bool four_letter_mapping_test(void)
{
    barcode_action_t mapping[BARCODE_ALPHABET] = {BARCODE_UNMAPPED};
    bool valid =
        !mission_mapping_valid(NULL) && !mission_mapping_valid(mapping);
    /* Every letter can occupy every motion slot; no fixed ABCD mapping. */
    for (unsigned letter = 0u; letter < BARCODE_ALPHABET; ++letter)
    {
        for (unsigned shift = 0u; shift < 4u; ++shift)
        {
            for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
            {
                mapping[i] = BARCODE_UNMAPPED;
            }
            for (unsigned slot = 0u; slot < 4u; ++slot)
            {
                mapping[(letter + slot) % BARCODE_ALPHABET] =
                    (barcode_action_t)(BARCODE_LEFT + (slot + shift) % 4u);
            }
            valid = mission_mapping_valid(mapping) && valid;
        }
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        mapping[i] = BARCODE_UNMAPPED;
    }
    mapping[0] = BARCODE_LEFT;
    mapping[1] = BARCODE_RIGHT;
    mapping[2] = BARCODE_STRAIGHT;
    mapping[3] = BARCODE_UNMAPPED; /* Missing U-turn. */
    valid = !mission_mapping_valid(mapping) && valid;
    mapping[3] = BARCODE_U_TURN;
    mapping[4] = BARCODE_LEFT; /* Fifth assigned letter/duplicate action. */
    valid = !mission_mapping_valid(mapping) && valid;
    mapping[4] = BARCODE_UNMAPPED;
    mapping[5] = (barcode_action_t)99; /* Invalid enum. */
    return !mission_mapping_valid(mapping) && valid;
}
void integration_test(void)
{
    TEST_INIT();
    RobotConfig cfg = fixture_config();
    bool positive = robot_config_validate(&cfg);
    cfg.braking_headroom_m = 0;
    bool invalid = !robot_config_validate(&cfg);
    cfg = fixture_config();
    cfg.course_confirmed = false;
    invalid = invalid && !robot_config_validate(&cfg);
    TEST_CHECK(
        "INT-15", positive && invalid && !robot_config_validate(NULL),
        "valid complete settings accepted; missing headroom/course rejected");
    cfg = fixture_config();
    TEST_CHECK("INT-16",
               mission_junction_check(&cfg, 0.1f, 20u) &&
                   !mission_junction_check(&cfg, 0.1f, 19u) &&
                   !mission_junction_check(&cfg, -1.0f, 1000u) &&
                   mission_finish_check(&cfg, 4u, 1.0f, 500u) &&
                   !mission_finish_check(&cfg, 3u, 1.0f, 500u),
               "positive junction/finish and travel/action/dwell boundaries");
    MissionDwell dwell = {0};
    S3IrSnapshot ir = {.normalized_valid = true,
                       .line = IR_LINE_BROAD_BLACK,
                       .sequence = UINT32_MAX,
                       .sampled_ms = UINT32_MAX - 10u};
    bool consecutive =
        mission_broad_observe(&dwell, &ir, ir.sampled_ms) == 0u &&
        mission_broad_observe(&dwell, &ir, 10u) == 0u;
    ir.sequence = 0u;
    ir.sampled_ms = 9u;
    consecutive = consecutive && mission_broad_observe(&dwell, &ir, 10u) == 20u;
    ir.sequence = 2u;
    ir.sampled_ms = 49u;
    consecutive = consecutive && mission_broad_observe(&dwell, &ir, 49u) == 0u;
    ir.line = IR_LINE_VISIBLE;
    consecutive = consecutive &&
                  mission_broad_observe(&dwell, &ir, 49u) == 0u &&
                  dwell.count == 0u;
    TEST_CHECK("INT-22", consecutive,
               "distinct consecutive acquisitions, cache and wrap");
    TEST_CHECK("INT-23",
               mission_obstacle_trigger(&cfg) + cfg.sonar_forward_m >
                   mission_pivot_radius(&cfg),
               "braking begins outside center-based pivot envelope");
    TEST_CHECK("INT-21", four_letter_mapping_test(),
               "any four letters, one per motion; 22 remain unmapped");
    ir = (S3IrSnapshot){.normalized_valid = true,
                        .capture_kind = IR_CAPTURE_NONE,
                        .sampled_ms = 100u};
    MotionFeedback motion = {.valid = true, .sampled_ms = 100u};
    TerrainSnapshot terrain = {
        .terrain_valid = true, .calibrated = true, .timestamp_ms = 100u};
    bool health = mission_sensors_ready(120u, &ir, &motion, &terrain) &&
                  !mission_sensors_ready(161u, &ir, &motion, &terrain) &&
                  !mission_sensors_ready(120u, NULL, &motion, &terrain);
    terrain.calibrated = false;
    health = health && !mission_sensors_ready(120u, &ir, &motion, &terrain);
    terrain.calibrated = true;
    terrain.state = TERRAIN_STATE_IMPACT;
    health = health && !mission_sensors_ready(120u, &ir, &motion, &terrain);
    terrain.state = TERRAIN_STATE_FLAT;
    motion.valid = false;
    health = health && !mission_sensors_ready(120u, &ir, &motion, &terrain);
    TEST_CHECK("INT-24", health,
               "start/scan resume requires all required producers healthy");
    TEST_CHECK("INT-25", polling_mutex_test(),
               "real kernel ceiling raises and restores polling lock priority");
    TEST_SUMMARY("integration");
}
#endif
