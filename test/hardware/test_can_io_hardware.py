from pathlib import Path
import unittest

from scripts.check_can_io_hardware import (
    CONNECTOR_PINOUT,
    validate_connector,
    validate_analog_inputs,
    validate_communications,
    validate_outputs,
    validate_bom,
    validate_layout,
    validate_documentation,
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

    def test_all_analog_channels_are_symmetric_and_reset_safe(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_analog_inputs(root))

    def test_communications_egt_and_flex_are_protected(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_communications(root))

    def test_outputs_default_safe_and_are_current_limited(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_outputs(root))

    def test_bom_is_complete_and_has_verified_constraints(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_bom(root))

    def test_layout_contains_required_zones_classes_and_safe_separation(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_layout(root))

    def test_bringup_and_exports_cover_all_release_gates(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_documentation(root))


if __name__ == "__main__":
    unittest.main()
