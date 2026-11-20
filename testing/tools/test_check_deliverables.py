"""Deliverables check against this repository and damaged copies of it."""
import sys
import shutil
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import check_deliverables as check

assert check.main([str(ROOT)]) == 0

with tempfile.TemporaryDirectory() as temporary:
    copy = Path(temporary) / "repo"
    for part in ("app_program", "scripts", "testing"):
        shutil.copytree(ROOT / part, copy / part,
                        ignore=shutil.ignore_patterns("*.o", "*.d"))
    shutil.copy2(ROOT / "team.md", copy / "team.md")
    assert check.main([str(copy)]) == 0

    # A missing test, an unwired test, a missing report and a removed API.
    def damaged(relative, old, new, expected):
        path = copy / relative
        original = path.read_text()
        assert old in original, old
        path.write_text(original.replace(old, new))
        assert check.main([str(copy)]) == 1, expected
        path.write_text(original)

    damaged("testing/tools/run.sh", "test_barcode", "test_other", "unwired")
    damaged("app_program/motion/README.md", "## Motion accuracy evaluation",
            "## Other", "report")
    damaged("app_program/obstacle/obstacle.h", "obstacle_scan_request",
            "obstacle_scan_ask", "API")
    # Subsystems that stop agreeing.
    damaged("scripts/mqtt_client.py", "TERRAIN_TRACE: 14", "TERRAIN_TRACE: 8",
            "frame length")
    damaged("app_program/comms/comms.c", "void comms_step(uint32_t now)\n{",
            "void comms_step(uint32_t now)\n{\n    (void)motion_stop();",
            "networking stops the robot")
    damaged("app_program/obstacle/obstacle.c", "ER obstacle_init(void)\n{",
            "ER obstacle_init(void)\n{\n    (void)drv_motor_brake();",
            "second motor writer")
print("deliverables check: PASS (repository, unwired test, missing report and "
      "API, frame length, networking and motor ownership)")
