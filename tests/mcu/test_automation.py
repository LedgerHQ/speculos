import json
import jsonschema
import os
import pkg_resources
import pytest

from speculos.mcu import automation


class TestAutomation:
    @staticmethod
    def get_json_path(name):
        path = os.path.join("resources", name)
        path = pkg_resources.resource_filename(__name__, path)
        return f"file:{path}"

    def test_valid_json(self):
        """Valid JSON complying with the schema."""

        automation.Automation('{"version": 1, "rules": []}')
        automation.Automation(TestAutomation.get_json_path("automation_valid.json"))

    def test_invalid_json(self):
        """Invalid JSON/schema testcases."""

        with pytest.raises(json.decoder.JSONDecodeError):
            assert automation.Automation("x")

        with pytest.raises(jsonschema.exceptions.ValidationError):
            assert automation.Automation("{}")

        names = [
            "automation_invalid_rule_key.json",
            "automation_invalid_action_name.json",
            "automation_invalid_action_args.json",
        ]
        for name in names:
            with pytest.raises(jsonschema.exceptions.ValidationError):
                assert automation.Automation(TestAutomation.get_json_path(name))

    def test_rules(self):
        expected_actions = [
            ["button", 2, True],
            ["button", 2, False],
            ["setbool", "seen", True],
            ["exit"]
        ]
        regexp_actions = [
            ["exit"]
        ]
        default_actions = [
            ["setbool", "default_match", True]
        ]

        auto = automation.Automation(TestAutomation.get_json_path("automation_valid.json"))
        assert auto.get_actions("Application", 0, 0) == default_actions
        assert auto.get_actions("Application", 35, 3) == expected_actions

        auto.set_bool("seen", True)
        assert auto.get_actions("Application", 35, 3) == default_actions

        auto.set_bool("seen", False)
        assert auto.get_actions("Application", 35, 3) == expected_actions

        assert auto.get_actions("1234", 35, 3) == regexp_actions
