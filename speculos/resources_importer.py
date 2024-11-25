import json
from pathlib import Path
from platform import python_version_tuple
from typing import Dict

major, minor, _ = python_version_tuple()
assert major == "3"

if int(minor) <= 8:
    import importlib_resources as resources
else:
    import importlib.resources as resources


def get_resources_path(module: str, filename: str) -> Path:
    return resources.files(__package__) / module / "resources" / filename


def get_resource_schema_as_json(module: str, filename: str) -> Dict:
    with get_resources_path(module, filename).open("rb") as fp:
        schema = json.load(fp)
    return schema
