from pathlib import Path
import unittest

from scripts.check_can_io_hardware import (
    CONNECTOR_PINOUT,
    validate_connector,
    validate_power_and_mcu,
    validate_project,
)


class CanIoHardwareContractTest(unittest.TestCase):
    def test_complete_project_contract(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_project(root))

    def test_connector_has_exact_24_pin_mapping_and_geometry(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_connector(root))
        self.assertEqual(list(range(1, 25)), sorted(CONNECTOR_PINOUT))

    def test_power_and_mcu_have_protected_chain_and_service_access(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_power_and_mcu(root))


if __name__ == "__main__":
    unittest.main()
