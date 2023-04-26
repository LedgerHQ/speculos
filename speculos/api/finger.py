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
        delay = args.get("delay", 0.1)

        if action == "press-and-release":
            self.seph.handle_finger(args["x"], args["y"], True)
            self.seph.handle_wait(delay)
            self.seph.handle_finger(args["x"], args["y"], False)
        else:
            self.seph.handle_finger(args["x"], args["y"], action == "press")

        return {}, 200
