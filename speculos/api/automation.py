import json
import jsonschema
from flask import request

from ..mcu import automation as mcu_automation
from .restful import SephResource


class Automation(SephResource):
    def post(self):
        document = request.get_data()

        try:
            document = document.decode("ascii")
        except UnicodeDecodeError:
            return "invalid encoding", 400

        if document.startswith("file:"):
            return "invalid document", 400

        try:
            rules = mcu_automation.Automation(document)
        except json.decoder.JSONDecodeError:
            return "invalid document", 400
        except jsonschema.exceptions.ValidationError:
            return "invalid document", 400

        self.seph.automation = rules

        return {}, 200
