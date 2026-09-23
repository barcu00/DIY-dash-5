from pathlib import Path
import unittest

from scripts.check_can_io_hardware import validate_project


class CanIoHardwareContractTest(unittest.TestCase):
    def test_complete_project_contract(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_project(root))


if __name__ == "__main__":
    unittest.main()
