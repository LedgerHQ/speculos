import jsonschema
from flask import request

from speculos.resources_importer import get_resource_schema_as_json
from .restful import SephResource


class Ticker(SephResource):
    schema = get_resource_schema_as_json("api", "ticker.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        action = args["action"]
        self.seph.handle_ticker_request(action)
        return {}, 200
