import pytest
import os
import re

from typing import List

from .apdu_client import APDUClient, AppInfo
from .finger_client import FingerClient

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


@pytest.fixture()
def finger_client():
    """
    Return a finger_tcp client
    """
    finger = FingerClient()
    yield finger
    # teardown
    if finger.running:
        finger.stop()
        finger.join()


def list_apps_to_test(app_dir) -> List[AppInfo]:
    """
    List apps matching the pattern:

        <device>#<app_name>#<sdk_version>#<commit_hash>.elf

    in the apps/ directory and return a list of APDUClient
    objects for these applications.

    A typical application path looks like:

    'apps/nanos#btc#1.5#5b6693b8.elf'
    """
    # name example: nanos#btc#1.5#5b6693b8.elf
    app_regexp = re.compile(
        r"(nanos|nanox|blue)#(.+)#([^#][\d\w\-.]+)#([a-f0-9]{8}).elf"
    )
    all_apps = []
    for filename in os.listdir(app_dir):
        matching = re.match(app_regexp, filename)
        if not matching or len(matching.groups()) != 4:
            continue

        all_apps.append(
            AppInfo(
                filepath=os.path.join(app_dir, filename),
                device=matching.group(1),
                name=matching.group(2),
                version=matching.group(3),
                hash=matching.group(4),
            )
        )
    return all_apps


@pytest.fixture(scope="function")
def app(request):
    _app = APDUClient(request.param)
    yield _app
    _app.stop()


def pytest_generate_tests(metafunc):
    # retrieve the list of apps in the ../apps directory
    app_dir = os.path.join(SCRIPT_DIR, os.pardir, os.pardir, "apps")
    apps = list_apps_to_test(app_dir)
    if not hasattr(metafunc.cls, "app_names") or not isinstance(
        metafunc.cls.app_names, list
    ):
        pytest.fail(
            "The TestClass {metafunc.cls} does not have a correct 'app_names' attribute. \n"
            "The 'app_names' attribute must contain a list of app names that will be"
            "tested by this test."
        )
    apps = [app for app in apps if app.name in metafunc.cls.app_names]

    # if a test function has an app parameter, give the list of app
    if "app" in metafunc.fixturenames:
        # test are run on each app
        metafunc.parametrize("app", apps, indirect=True, scope="function")
