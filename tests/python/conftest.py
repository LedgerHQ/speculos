import pytest
import os
import re
from collections import namedtuple
from pathlib import Path
from typing import List

from speculos.client import SpeculosClient


# going back from...(conftest.py \ python \ tests \ git root) / 'apps'
APP_DIR = Path(__file__).resolve().parent.  parent. parent    / "apps"
AppInfo = namedtuple("AppInfo", ["filepath", "model", "name", "sdk", "hash"])


def app_info_from_path(path: Path) -> AppInfo:
    # name example: nanos#btc#1.5#5b6693b8.elf
    app_regexp = re.compile(r"^(nanos|nanox|blue|nanosp)#([^#]+)#([^#][\d\w\-.]+)#([a-f0-9]*)\.elf$")
    matching = re.match(app_regexp, path.name)
    if not matching:
        return None
    assert len(matching.groups()) == 4
    return AppInfo(filepath=path, model=matching.group(1), name=matching.group(2), sdk=matching.group(3),
                   hash=matching.group(4))


def list_apps_to_test() -> List[AppInfo]:
    """
    List apps matching the pattern:

        <device>#<app_name>#<sdk_version>#<commit_hash>.elf

    in the apps/ directory and return a list of APDUClient
    objects for these applications.

    A typical application path looks like:

    'apps/nanos#btc#1.5#5b6693b8.elf'
    """
    all_apps = []
    for appfile in APP_DIR.iterdir():
        if "#" not in appfile.name:
            continue
        info = app_info_from_path(appfile)
        if not info:
            pytest.fail(
                f"An unexpected file was found in apps/, with a # but not matching the pattern: {filename!r}"
            )
            continue
        all_apps.append(info)
    return all_apps


@pytest.fixture(scope="function")
def app(request, client):
    return app_info_from_path(Path(client.app))


def get_apps(name: str) -> List[AppInfo]:
    """Retrieve the list of apps in the ../../apps directory."""
    return [app for app in list_apps_to_test() if app.name == name]


def default_btc_app() -> List[AppInfo]:
    filepath = (APP_DIR / "btc.elf").resolve()
    apps = get_apps("btc")
    return [app for app in apps if app.filepath == filepath]


def idfn(app: Path) -> str:
    """
    Set the test ID to the app file name for each test running on a set of apps.

    From https://docs.pytest.org/en/6.2.x/example/parametrize.html#different-options-for-test-ids:

      These IDs can be used with -k to select specific cases to run, and they will
      also identify the specific case when one is failing.
    """
    return app.filepath


def client_instance(app, additional_args=None):
    args = ["--model", app.model, "--sdk", app.sdk]
    if additional_args is not None:
        args += additional_args
    return SpeculosClient(str(app.filepath), args=args)


@pytest.fixture(scope="module", params=get_apps("btc"), ids=idfn)
def client_btc(request):
    with client_instance(request.param) as _client:
        yield _client


@pytest.fixture(scope="module", params=get_apps("btc-test"), ids=idfn)
def client_btc_testnet(request):
    app = request.param
    btc_app = app.filepath.parent / app.filepath.name.replace("btc-test", "btc")
    assert btc_app.is_file()
    args = ["-l", "Bitcoin:%s" % str(btc_app)]

    with client_instance(request.param, additional_args=args) as _client:
        yield _client


@pytest.fixture(scope="module", params=get_apps("ram-page"), ids=idfn)
def client_ram_page(request):
    with client_instance(request.param) as _client:
        yield _client


@pytest.fixture(scope="function", params=default_btc_app(), ids=idfn)
def client_vnc(request):
    # Pytest has changed its API in version 4: https://github.com/pytest-dev/pytest/pull/4564
    if hasattr(request.node, "get_closest_marker"):
        get_closest_marker = request.node.get_closest_marker
    else:
        get_closest_marker = request.node.get_marker
    args = list(get_closest_marker("additional_args").args)
    with client_instance(request.param, args) as _client:
        yield _client


@pytest.fixture(scope="class")
def client(request):
    """Run the API tests on the default btc.elf app."""

    info = app_info_from_path((APP_DIR / "btc.elf").resolve())
    args = ["--model", info.model, "--sdk", info.sdk]
    with SpeculosClient(app=str(info.filepath), args=args) as _client:
        yield _client
