import json
import os.path
import pkg_resources


def load_json_schema(filename):
    path = os.path.join("resources", filename)
    with pkg_resources.resource_stream(__name__, path) as fp:
        schema = json.load(fp)
    return schema
