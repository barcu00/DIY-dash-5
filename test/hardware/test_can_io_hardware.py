from pathlib import Path
import json
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

    def test_critical_pin_map_is_complete_and_datasheet_traceable(self):
        root = Path(__file__).resolve().parents[2]
        manifest_path = root / "hardware/can-io-module/electrical-pin-map.json"
        self.assertTrue(manifest_path.is_file(), "missing machine-readable electrical pin map")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        for component in ("U1", "U2", "U3", "U4", "U5", "U6", "U7", "U8", "U9", "U10", "U11"):
            self.assertIn(component, manifest["components"])
            self.assertRegex(manifest["components"][component]["datasheet"], r"^https://")
            self.assertTrue(manifest["components"][component]["pins"])

        mcu = manifest["components"]["U1"]
        self.assertEqual("STM32G0B1CBT6", mcu["part"])
        self.assertEqual("VBAT", mcu["pins"]["4"])
        self.assertEqual("VREF+", mcu["pins"]["5"])
        self.assertEqual("VDD/VDDA", mcu["pins"]["6"])
        self.assertEqual("VSS/VSSA", mcu["pins"]["7"])
        self.assertEqual("PF2-NRST", mcu["pins"]["10"])
        self.assertEqual("PA13-SWDIO", mcu["pins"]["35"])
        self.assertEqual("PA14-SWCLK", mcu["pins"]["36"])
        self.assertEqual(set(map(str, range(1, 49))), set(mcu["pins"]))

        assignments = manifest["mcu_assignments"]
        used_pins = [entry["pin"] for entry in assignments.values()]
        self.assertEqual(len(used_pins), len(set(used_pins)), "MCU assignments must not reuse a physical pin")
        for signal in (
            "ADC_IN1", "ADC_IN2", "ADC_IN3", "ADC_IN4", "ADC_IN5", "ADC_IN6", "ADC_IN7", "ADC_IN8",
            "DAC1_RAW", "DAC2_RAW", "CAN_RX", "CAN_TX", "KTX", "KRX", "EGT_SCK", "EGT_CS", "EGT_SO",
            "FLEX_CAPTURE", "SWDIO", "SWCLK", "NRST", "VBAT_MON", "V5_MON", "V3V3_MON",
        ):
            self.assertIn(signal, assignments)

        self.assertEqual(
            {"1": "+3V3", "2": "SWDIO", "3": "SWCLK", "4": "NRST", "5": "POWER_GND"},
            manifest["connectors"]["J2"]["pins"],
        )

        self.assertEqual(
            {"1": "RX", "2": "LO", "3": "VCC", "4": "TX", "5": "GND", "6": "K", "7": "VS", "8": "LI"},
            manifest["components"]["U7"]["pins"],
        )
        self.assertEqual(
            {"1": "TXD", "2": "GND", "3": "VCC", "4": "RXD", "5": "VIO", "6": "CANL", "7": "CANH", "8": "S"},
            manifest["components"]["U8"]["pins"],
        )
        self.assertEqual(
            {"1": "GND", "2": "T-", "3": "T+", "4": "VCC", "5": "SCK", "6": "CS", "7": "SO", "8": "NC"},
            manifest["components"]["U10"]["pins"],
        )

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
