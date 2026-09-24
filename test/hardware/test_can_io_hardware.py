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
    def test_schematics_are_electrical_not_text_only(self):
        root = Path(__file__).resolve().parents[2]
        schematic_root = root / "hardware/can-io-module"
        paths = [schematic_root / "can-io-module.kicad_sch"] + sorted(
            (schematic_root / "sheets").glob("*.kicad_sch")
        )
        texts = [path.read_text(encoding="utf-8") for path in paths]
        instance_count = sum(len(re.findall(r'\n\s*\(symbol\s*\n\s*\(lib_id\s+', text)) for text in texts)
        wire_count = sum(len(re.findall(r'\n\s*\(wire\s*\n', text)) for text in texts)
        label_count = sum(len(re.findall(r'\n\s*\((?:global_label|hierarchical_label|label)\s+', text)) for text in texts)
        self.assertGreaterEqual(instance_count, 70, "schematics must contain real component instances")
        self.assertGreaterEqual(wire_count, 80, "schematics must contain real electrical wires")
        self.assertGreaterEqual(label_count, 40, "schematics must contain real electrical labels")

    def test_schematic_and_pcb_references_have_functional_parity(self):
        root = Path(__file__).resolve().parents[2]
        base = root / "hardware/can-io-module"
        schematic_text = "\n".join(
            path.read_text(encoding="utf-8")
            for path in [base / "can-io-module.kicad_sch", *sorted((base / "sheets").glob("*.kicad_sch"))]
        )
        pcb_text = (base / "can-io-module.kicad_pcb").read_text(encoding="utf-8")
        schematic_refs = set(re.findall(r'\(property\s+"Reference"\s+"([A-Z]+\d+)"', schematic_text))
        pcb_refs = set(re.findall(r'\(property\s+"Reference"\s+"([A-Z]+\d+)"', pcb_text))
        ignored = {"H1", "H2", "H3", "H4"}
        self.assertGreaterEqual(len(schematic_refs), 70)
        self.assertEqual(schematic_refs, pcb_refs - ignored)

    def test_can_common_mode_choke_is_four_terminal_or_bypassed(self):
        root = Path(__file__).resolve().parents[2]
        pcb = (root / "hardware/can-io-module/can-io-module.kicad_pcb").read_text(encoding="utf-8")
        l2 = next(block for block in _sexpr_blocks(pcb, "footprint") if '(property "Reference" "L2"' in block)
        pads = re.findall(r'\(pad\s+"(\d+)"', l2)
        self.assertEqual(["1", "2", "3", "4"], sorted(pads))
        for net in ("CAN_H_PROTECTED", "CAN_L_PROTECTED", "CAN_H", "CAN_L"):
            self.assertIn(f'"{net}"', l2)

    def test_swd_header_is_1x5_254mm_with_exact_pinout(self):
        root = Path(__file__).resolve().parents[2]
        pcb = (root / "hardware/can-io-module/can-io-module.kicad_pcb").read_text(encoding="utf-8")
        j2 = next(block for block in _sexpr_blocks(pcb, "footprint") if '(property "Reference" "J2"' in block)
        expected = {
            1: "+3V3",
            2: "SWDIO",
            3: "SWCLK",
            4: "NRST",
            5: "POWER_GND",
        }
        for pin, net in expected.items():
            self.assertRegex(j2, rf'\(pad\s+"{pin}"\s+.*?\(net\s+\d+\s+"{re.escape(net)}"\)')
        self.assertIn("2.54", j2)
        self.assertIn("PIN 1", j2)

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
