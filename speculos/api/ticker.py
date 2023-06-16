import jsonschema
from flask import request

from .helpers import load_json_schema
from .restful import SephResource


class Ticker(SephResource):
    schema = load_json_schema("ticker.schema")

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        action = args["action"]
        self.seph.handle_ticker_request(action)
        return {}, 200
