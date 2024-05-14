import socket
import threading
from typing import Any, Dict
from flask import Flask
from flask_restful import Api

from speculos.mcu.display import DisplayNotifier, IODevice
from speculos.mcu.readerror import ReadError
from speculos.mcu.seproxyhal import SeProxyHal
from speculos.observer import BroadcastInterface
from speculos.resources_importer import resources
from .apdu import APDU
from .automation import Automation
from .button import Button
from .events import Events
from .finger import Finger
from .screenshot import Screenshot
from .swagger import Swagger
from .web_interface import WebInterface
from .ticker import Ticker


class ApiRunner(IODevice):
    """Run the Speculos API server in a dedicated thread, with a notification when it stops"""
    def __init__(self, api_port: int) -> None:
        self._api_wrapper: ApiWrapper
        # self.sock is used by Screen.add_notifier. Closing self._notify_exit
        # signals it that the API is no longer running.
        self.sock: socket.socket
        self._notify_exit: socket.socket
        self.sock, self._notify_exit = socket.socketpair()
        self._port: int = api_port
        self._api_thread: threading.Thread

    @property
    def file(self):
        return self.sock

    def can_read(self, screen: DisplayNotifier) -> None:
        # Being able to read from the socket only happens when the API server exited.
        raise ReadError("API server exited")

    def start_server_thread(self,
                            screen_: DisplayNotifier,
                            seph_: SeProxyHal,
                            automation_server: BroadcastInterface) -> None:
        self._api_wrapper = ApiWrapper(self._port, screen_, seph_, automation_server)
        self._api_thread = threading.Thread(target=self._api_wrapper.run, name="API-server", daemon=True)
        self._api_thread.start()

    def stop(self):
        self._notify_exit.close()


class ApiWrapper:
    def __init__(self,
                 api_port: int,
                 screen: DisplayNotifier,
                 seph: SeProxyHal,
                 automation_server: BroadcastInterface):
        self._port = api_port
        static_folder = str(resources.files(__package__) / "static")
        self._app = Flask(__name__, static_url_path="", static_folder=static_folder)
        self._app.env = "development"

        screen_kwargs = {"screen": screen}
        seph_kwargs = {"seph": seph}
        app_kwargs = {"app": self._app}
        event_kwargs: Dict[str, Any] = {**app_kwargs, "automation_server": automation_server}

        self._api = Api(self._app)

        self._api.add_resource(APDU, "/apdu", resource_class_kwargs=seph_kwargs)
        self._api.add_resource(Automation, "/automation", resource_class_kwargs=seph_kwargs)
        self._api.add_resource(Button,
                               "/button/left",
                               "/button/right",
                               "/button/both",
                               resource_class_kwargs=seph_kwargs)
        self._api.add_resource(Events, "/events", resource_class_kwargs=event_kwargs)
        self._api.add_resource(Finger, "/finger", resource_class_kwargs=seph_kwargs)
        self._api.add_resource(Screenshot, "/screenshot", resource_class_kwargs=screen_kwargs)
        self._api.add_resource(Swagger, "/swagger/", resource_class_kwargs=app_kwargs)
        self._api.add_resource(WebInterface, "/", resource_class_kwargs=app_kwargs)
        self._api.add_resource(Ticker, "/ticker/", resource_class_kwargs=seph_kwargs)

    def run(self):
        # threaded must be set to allow serving requests along events streaming
        self._app.run(host="0.0.0.0", port=self._port, threaded=True, use_reloader=False)
