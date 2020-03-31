import pytest
import os
import pathlib
from apdu_client import APDUClient
from finger_client import FingerClient

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

@pytest.fixture(scope="function")
def stop_app(request):
    yield
    app = request.getfixturevalue('app')
    app.stop()


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


def listapps(app_dir):
    '''List every available apps in the app/ directory.'''

    paths = [ filename for filename in os.listdir(app_dir) if pathlib.Path(filename).suffix == '.elf' ]
    # ignore app whose name doesn't match the expected pattern
    paths = [ path for path in paths if path.count('#') == 3 ]
    return [ APDUClient(os.path.join(app_dir, path)) for path in paths ]

def filter_apps(cls, apps):
    '''
    Filter apps by the class name of the test.

    If no filter matches the class name, all available apps are returned.
    '''

    app_filter = {
        'TestBtc': [ 'btc' ],
        'TestBtcTestnet': [ 'btc-test' ],
        'TestVault': [ 'vault' ],
        'TestRamPage': ['ram-page']
    }

    class_name = cls.__name__.split('.')[-1]
    if class_name in app_filter:
        names = app_filter[class_name]
        apps = [ app for app in apps if app.name in names ]

    return apps

def pytest_generate_tests(metafunc):
    # retrieve the list of apps in the ../apps directory
    app_dir = os.path.join(SCRIPT_DIR, '..', '..', 'apps')
    apps = listapps(app_dir)
    apps = filter_apps(metafunc.cls, apps)

    # if a test function has an app parameter, give the list of app
    if 'app' in metafunc.fixturenames:
        # test are run on each app
        metafunc.parametrize('app', apps, scope='function')
