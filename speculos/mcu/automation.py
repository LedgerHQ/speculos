from dataclasses import dataclass
import json
import jsonschema
import logging
import os
import pkg_resources
import re


@dataclass
class TextEvent:
    text: str
    x: int
    y: int


class Automation:
    def __init__(self, document):
        self.logger = logging.getLogger("automation")
        self.variables = {}

        if document.startswith("file:"):
            path = document[5:]
            with open(path) as fp:  # lgtm [py/path-injection]
                self.json = json.load(fp)
        else:
            self.json = json.loads(document)
        self.validate()

    def validate(self):
        path = os.path.join("resources", "automation.schema")
        with pkg_resources.resource_stream(__name__, path) as fp:
            schema = json.load(fp)
        jsonschema.validate(instance=self.json, schema=schema)

    def set_bool(self, key, value):
        self.variables[key] = value

    def get_actions(self, text, x, y):
        self.logger.debug(f'getting actions for "{text}" ({x}, {y})')

        for rule in self.json["rules"]:
            if "text" in rule and rule["text"] != text:
                continue
            if "regexp" in rule and not re.match(rule["regexp"], text):
                continue
            if "x" in rule and rule["x"] != x:
                continue
            if "y" in rule and rule["y"] != y:
                continue
            if "conditions" in rule:
                condition = True
                for key, value in rule["conditions"]:
                    if self.variables.get(key, False) != value:
                        condition = False
                        break
                if not condition:
                    continue

            if "actions" not in rule:
                self.logger.warning(f'missing "actions" key for rule {rule}')
                continue

            return rule["actions"]

        return []
