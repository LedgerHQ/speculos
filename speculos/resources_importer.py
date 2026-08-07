import json
from pathlib import Path
from platform import python_version_tuple

major, minor, _ = python_version_tuple()
if major != "3":
    raise ValueError("Python 3 is required")

if int(minor) <= 8:
    import importlib_resources as resources
else:
    import importlib.resources as resources


def get_resources_path(module: str, filename: str) -> Path:
    return resources.files(__package__) / module / "resources" / filename


def get_resource_schema_as_json(module: str, filename: str) -> dict:
    with get_resources_path(module, filename).open("rb") as fp:
        schema = json.load(fp)
    return schema
