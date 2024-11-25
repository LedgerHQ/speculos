import jsonschema
from flask import request

from speculos.resources_importer import get_resource_schema_as_json
from .restful import SephResource


class Finger(SephResource):
    schema = get_resource_schema_as_json("api", "finger.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        action = args["action"]
        delay = args.get("delay", 0.1)

        x1, y1 = args["x"], args["y"]
        if action == "press-and-release":
            self.seph.handle_finger(x1, y1, True)
            self.seph.handle_wait(delay)
            x2, y2 = args.get("x2", x1), args.get("y2", y1)
            self.seph.handle_finger(x2, y2, False)
        else:
            self.seph.handle_finger(x1, y1, action == "press")

        return {}, 200
