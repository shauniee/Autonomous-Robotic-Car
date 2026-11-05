#!/usr/bin/env python3
"""Host-only regression cases for physical-run diagnostics."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import triage_mqtt_capture


class CaptureTests(unittest.TestCase):
    def test_no_echo_is_diagnostic_not_a_stop(self):
        lines = [
            "TELEMETRY #9 state=FOLLOW_LINE motion=RUNNING line=VISIBLE speed L=100 R=98 mm/s "
            "| humps=1 max=10 mm | front=no echo | barcode=A->LEFT",
            "EVENT BARCODE: letter A -> LEFT at t=50 ms, counts L=100 R=100",
            "WHEEL {\"measured_left\":100,\"measured_right\":99}",
            "TELEMETRY #10 state=STOPPED motion=IDLE line=VISIBLE speed L=1935 R=-222 mm/s "
            "| humps=1 max=10 mm | front=no echo | barcode=A->LEFT",
        ]
        result = triage_mqtt_capture.analyse(lines)
        self.assertEqual(result["counts"]["telemetry_no_echo"], 2)
        self.assertEqual(result["counts"]["state_changes"], 1)
        self.assertEqual(result["events"]["BARCODE"], 1)
        reasons = {x["reason"] for x in result["anomalies"]}
        self.assertIn("entered_STOPPED", reasons)
        self.assertIn("averaged_speed_over_limit", reasons)

    def test_empty_and_malformed_capture(self):
        result = triage_mqtt_capture.analyse(["nonsense", "WHEEL {bad}", "WHEEL []",
                                            'WHEEL {"measured_left":NaN}'])
        self.assertEqual(result["counts"]["undecodable_wheel_traces"], 3)
        self.assertEqual(result["first_and_last_hump_counts"], [None, None])

    def test_missing_speed_keeps_state_and_hump_evidence(self):
        lines = [f"[time] TELEMETRY #{seq} state={state} speed n/a | humps=2 "
                 "| front=no echo | barcode=A->LEFT" for seq, state in enumerate(
                     ("FOLLOW_LINE", "REACQUIRE", "STOPPED", "FAULT"))]
        lines += ['[time] WHEEL {"sampled_ms":123,"measured_left":201,"measured_right":null}']
        result = triage_mqtt_capture.analyse(lines)
        self.assertEqual(result["counts"]["telemetry_frames"], 4)
        self.assertEqual(result["counts"]["telemetry_speed_unavailable"], 4)
        self.assertEqual(result["counts"]["state_changes"], 3)
        self.assertEqual(result["first_and_last_hump_counts"], [2, 2])
        self.assertEqual(result["peak_wheel_speed_mmps"]["sampled_ms"], 123)
        self.assertEqual(result["peak_wheel_speed_mmps"]["value"], 201)


if __name__ == "__main__":
    unittest.main()
