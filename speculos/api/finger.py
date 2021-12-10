import time
import jsonschema
from flask import request

from .helpers import load_json_schema
from .restful import SephResource


class Finger(SephResource):
    schema = load_json_schema("finger.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        action = args["action"]
        actions = {"press": [True], "release": [False], "press-and-release": [True, False]}
        delay = args.get("delay", 0.1)

        for a in actions[action]:
            self.seph.handle_finger(args["x"], args["y"], a)
            if action == "press-and-release":
                time.sleep(delay)

        return {}, 200
