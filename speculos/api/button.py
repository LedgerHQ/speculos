import time
import jsonschema
from flask import request

from .helpers import load_json_schema
from .restful import SephResource


class Button(SephResource):
    schema = load_json_schema("button.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        button = request.base_url.split("/")[-1]
        buttons = {"left": [1], "right": [2], "both": [1, 2]}
        action = args["action"]
        actions = {"press": [True], "release": [False], "press-and-release": [True, False]}
        delay = args.get("delay", 0.1)

        for a in actions[action]:
            for b in buttons[button]:
                self.seph.handle_button(b, a)
            if action == "press-and-release":
                time.sleep(delay)

        return {}, 200
