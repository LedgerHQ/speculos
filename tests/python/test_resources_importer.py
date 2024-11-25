import speculos.resources_importer as r


def test_api_resource_exists():
    assert r.get_resources_path("api", "").exists()


def test_mcu_resource_exists():
    assert r.get_resources_path("mcu", "").exists()


def test_load_json():
    schema = r.get_resource_schema_as_json("api", "finger.schema")
    assert isinstance(schema, dict)
