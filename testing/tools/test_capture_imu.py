"""Host checks for IMU console capture; no sensor measurements."""

import sys
import io
from pathlib import Path
import unittest
from unittest.mock import patch


sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
import capture_imu as imu


def row(sequence=1, dropped=0):
    return f"[IMU_RAW] {sequence},2000,-10,87,492,1000,20,21,1,1,1,-500,0,0,0,120,{dropped}\n"


class CaptureTests(unittest.TestCase):
    def test_signed_axes_and_column_order(self):
        output = io.StringIO()
        self.assertEqual(imu.capture(["[IMU] calibration passed\n", row(), row(2)], output), 0)
        self.assertTrue(output.getvalue().startswith(",".join(imu.FIELDS)))
        self.assertIn("1,2000,-10,87,492,1000,20,21", output.getvalue())
        values = dict(zip(imu.FIELDS, imu.parse_row(row())))
        self.assertEqual(values["roll_cdeg"], -500)
        self.assertEqual(values["stationary"], 0)

    def test_loss_is_reported(self):
        self.assertEqual(imu.capture([row(), row(3)], io.StringIO()), 1)
        self.assertEqual(imu.capture([row(dropped=4)], io.StringIO()), 1)
        self.assertEqual(imu.capture(["[IMU_RAW] 1,2\n"], io.StringIO()), 1)
        self.assertEqual(imu.capture(["[IMU_RAW] bad," + ",".join(["0"] * 16)], io.StringIO()), 1)
        self.assertEqual(imu.capture([], io.StringIO()), 1)

    def test_sequence_wrap(self):
        self.assertEqual(imu.capture([row(0xFFFFFFFF), row(0)], io.StringIO()), 0)

    def test_serial_timeout_does_not_split_a_row(self):
        class Device:
            def __enter__(self):
                self.parts = iter([row().encode()[:20], row().encode()[20:]])
                return self

            def __exit__(self, *args):
                pass

            def read_until(self, *args, **kwargs):
                return next(self.parts)

        import types
        with patch.dict("sys.modules", {"serial": types.SimpleNamespace(Serial=lambda *a, **k: Device())}):
            with patch.object(imu.time, "monotonic", side_effect=[0, 0, 0.5, 2]):
                self.assertEqual(list(imu.serial_lines("COM_TEST", 1)), [row().rstrip("\n")])


if __name__ == "__main__":
    unittest.main()
