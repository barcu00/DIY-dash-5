import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class DeferredSettingsSaveContractTests(unittest.TestCase):
    def test_ui_callbacks_do_not_write_configuration_storage(self):
        source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")

        self.assertNotIn("repository_->saveCandidate", source)
        self.assertNotIn("repository_->resetLayout", source)
        self.assertNotIn("repository_->reset(", source)
        self.assertNotIn("persistSettings(", source)

    def test_application_loop_services_the_deferred_commit(self):
        header = (ROOT / "src/ui/ui.h").read_text(encoding="utf-8")
        source = (ROOT / "src/app/app.cpp").read_text(encoding="utf-8")

        self.assertIn("takeConfigCommit", header)
        self.assertIn("completeConfigCommit", header)
        self.assertIn("ui_.takeConfigCommit", source)
        self.assertIn("config_repository_.saveCandidate", source)
        self.assertIn("ui_.completeConfigCommit", source)

    def test_every_settings_exit_can_queue_dirty_values(self):
        source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")

        self.assertIn("queueSettingsOnExit", source)
        self.assertGreaterEqual(source.count("queueSettingsOnExit()"), 3)


if __name__ == "__main__":
    unittest.main()
