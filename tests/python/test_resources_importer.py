import speculos.resources_importer as r


def test_api_resource_exists():
    if not r.get_resources_path("api", "").exists():
        raise AssertionError("API resource does not exist")


def test_mcu_resource_exists():
    if not r.get_resources_path("mcu", "").exists():
        raise AssertionError("MCU resource does not exist")


def test_load_json():
    schema = r.get_resource_schema_as_json("api", "finger.schema")
    if not isinstance(schema, dict):
        raise AssertionError("Loaded JSON schema is not a dictionary")
