import importlib.resources
import json


def load_json_schema(filename):
    path = importlib.resources.files(__package__) / "resources" / filename
    with path.open("rb") as fp:
        schema = json.load(fp)
    return schema
