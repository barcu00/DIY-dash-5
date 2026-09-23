from pathlib import Path
import re
import unittest

from scripts.check_can_io_hardware import (
    _sexpr_blocks,
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

    def test_layout_is_a_real_routed_board_not_a_placement_mockup(self):
        root = Path(__file__).resolve().parents[2]
        pcb = root / "hardware/can-io-module/can-io-module.kicad_pcb"
        text = pcb.read_text(encoding="utf-8")
        self.assertNotIn("PLACEMENT_BLOCK", text)
        self.assertGreaterEqual(len(re.findall(r'\(footprint\b', text)), 70)
        self.assertGreaterEqual(len(re.findall(r'\(pad\b', text)), 180)
        self.assertGreaterEqual(len(re.findall(r'\(segment\b', text)), 120)

    def test_layout_stays_two_layer_and_qfp_pads_do_not_overlap(self):
        root = Path(__file__).resolve().parents[2]
        pcb = root / "hardware/can-io-module/can-io-module.kicad_pcb"
        text = pcb.read_text(encoding="utf-8")
        copper_layers = re.findall(r'\(\d+ "[FB]\.Cu" signal\)', text)
        self.assertEqual(2, len(copper_layers))
        self.assertEqual(1, len(re.findall(r'\(net_name\s+"POWER_GND"\).*?\(layer\s+"F\.Cu"\)', text, re.DOTALL)))
        self.assertEqual(1, len(re.findall(r'\(net_name\s+"POWER_GND"\).*?\(layer\s+"B\.Cu"\)', text, re.DOTALL)))
        self.assertRegex(text, r'\(connect_pads\s+yes\s+\(clearance\s+0\.25\)')
        u1 = next(block for block in _sexpr_blocks(text, "footprint") if '(property "Reference" "U1"' in block)
        self.assertRegex(u1, r'\(size\s+1\.2(?:0)?\s+0\.25\)')
        self.assertRegex(u1, r'\(size\s+0\.25\s+1\.2(?:0)?\)')
        self.assertNotRegex(u1, r'\(size\s+1\.15\s+0\.75\)')

    def test_autorouter_import_refills_ground_plane(self):
        root = Path(__file__).resolve().parents[2]
        bridge = (root / "scripts/autoroute_can_io.py").read_text(encoding="utf-8")
        self.assertIn("BuildConnectivity", bridge)
        self.assertIn("ZONE_FILLER", bridge)
        self.assertIn("Fill(board.Zones())", bridge)
        self.assertIn('remove_router_planes(args.exchange, "POWER_GND")', bridge)
        self.assertLess(bridge.index("ExportSpecctraDSN"), bridge.index("remove_router_planes(args.exchange"))

    def test_bringup_and_exports_cover_all_release_gates(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual([], validate_documentation(root))


if __name__ == "__main__":
    unittest.main()
