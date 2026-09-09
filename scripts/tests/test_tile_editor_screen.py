import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class TileEditorScreenContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "src/ui/ui.h").read_text(encoding="utf-8")
        cls.source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")
        cls.open_editor = cls.source.split("void Ui::openEditor", 1)[1]
        cls.open_editor = cls.open_editor.split(
            "void Ui::loadEditorTemperatureControls", 1
        )[0]
        cls.editor_event = cls.source.split("void Ui::editorEvent", 1)[1]
        cls.editor_event = cls.editor_event.split("void Ui::settingsEvent", 1)[0]

    def test_editor_is_an_independent_full_screen(self):
        self.assertIn("editor_screen_", self.header)
        self.assertIn("lv_obj_create(nullptr)", self.open_editor)
        self.assertIn("lv_scr_load(editor_screen_)", self.open_editor)
        self.assertNotIn("lv_layer_top()", self.open_editor)

    def test_parameter_selection_uses_profile_aware_id_mapping(self):
        self.assertIn("editor_parameter_options_", self.header)
        self.assertIn("ParameterOptions::build", self.open_editor)
        self.assertIn("parameterAt", self.editor_event)
        self.assertNotRegex(
            self.editor_event,
            r"static_cast<ParameterId>\s*\(\s*lv_dropdown_get_selected",
        )

    def test_editor_exposes_flag_active_color(self):
        self.assertIn("editor_flag_color_", self.header)
        self.assertIn('"YELLOW\\nGREEN\\nRED"', self.open_editor)

    def test_ui_strings_are_english_only(self):
        ui_files = list((ROOT / "src/ui").glob("*.cpp"))
        string_literal = re.compile(r'"(?:\\.|[^"\\])*"')
        forbidden = "ąćęłńóśźżĄĆĘŁŃÓŚŹŻ"
        for path in ui_files:
            source = path.read_text(encoding="utf-8")
            for literal in string_literal.findall(source):
                self.assertFalse(
                    any(character in literal for character in forbidden),
                    f"Non-English UI string in {path.name}: {literal}",
                )


if __name__ == "__main__":
    unittest.main()
