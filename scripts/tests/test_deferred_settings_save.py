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

    def test_settings_home_navigation_can_retry_a_failed_commit(self):
        source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")
        nav = source.split("void Ui::navEvent", 1)[1]
        nav = nav.split("void Ui::tileEvent", 1)[0]

        self.assertIn("current_page_ == Page::Settings", nav)
        self.assertNotIn("category() != SettingsCategory::Home", nav)

    def test_commit_feedback_uses_the_shared_top_layer(self):
        header = (ROOT / "src/ui/ui.h").read_text(encoding="utf-8")
        source = (ROOT / "src/ui/ui.cpp").read_text(encoding="utf-8")

        self.assertIn("commit_toast_", header)
        self.assertIn("showCommitFeedback", source)
        self.assertIn("lv_layer_top()", source)


if __name__ == "__main__":
    unittest.main()
