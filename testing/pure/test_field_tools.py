#!/usr/bin/env python3
"""Host-only regression cases for physical-run diagnostics and rate scoring."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import mission_trial_score
import triage_mqtt_capture


class TrialScoreTests(unittest.TestCase):
    def row(self, num, result, trace="1", firmware="abc", image="sha1"):
        return dict(trial_id=str(num), success=result, trace=trace,
                    firmware_sha=firmware, uf2_sha256=image, notes="")

    def test_95_percent_requires_20_recorded_laps(self):
        rows = [self.row(i, "PASS" if i < 20 else "FAIL") for i in range(1, 21)]
        scored = mission_trial_score.summarize(rows)
        self.assertEqual(scored[0]["attempts"], 20)
        self.assertEqual(scored[0]["success_rate_pct"], 95.0)
        self.assertTrue(scored[0]["target_met_in_sample"])
        self.assertFalse(mission_trial_score.summarize(rows[:19])[0]["target_met_in_sample"])

    def test_separate_firmware_and_trace_profiles(self):
        rows = [self.row(1, "PASS", "1"), self.row(1, "FAIL", "0"),
                self.row(1, "PASS", "1", image="sha2")]
        self.assertEqual(len(mission_trial_score.summarize(rows)), 3)

    def test_refuse_duplicate_and_undetermined_results(self):
        with self.assertRaises(ValueError):
            mission_trial_score.summarize([self.row(1, "PASS"), self.row(1, "FAIL")])
        with self.assertRaises(ValueError):
            mission_trial_score.summarize([self.row(1, "UNKNOWN")])


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
        result = triage_mqtt_capture.analyse(["nonsense", "WHEEL {bad}"])
        self.assertEqual(result["counts"]["undecodable_wheel_traces"], 1)
        self.assertEqual(result["first_and_last_hump_counts"], [None, None])


if __name__ == "__main__":
    unittest.main()
