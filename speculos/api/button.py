import jsonschema
from flask import request

from speculos.resources_importer import get_resource_schema_as_json
from .restful import SephResource


class Button(SephResource):
    schema = get_resource_schema_as_json("api", "button.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        button = request.base_url.split("/")[-1]
        buttons = {"left": [1], "right": [2], "both": [1, 2]}
        action = args["action"]
        delay = args.get("delay", 0.1)

        if action == "press-and-release":
            for b in buttons[button]:
                self.seph.handle_button(b, True)
            self.seph.handle_wait(delay)
            for b in buttons[button]:
                self.seph.handle_button(b, False)
            # Some app tests rely on this delay to make sure the screen is updated
            # and the app is ready to process next button press.
            # This is dangerous but we keep it as is to not break everything.
            self.seph.handle_wait(delay)
        else:
            for b in buttons[button]:
                self.seph.handle_button(b, action == "press")

        return {}, 200
