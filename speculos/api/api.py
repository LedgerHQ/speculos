import socket
import threading
import pkg_resources
from typing import Any, Dict
from flask import Flask
from flask_restful import Api

from speculos.mcu.display import DisplayNotifier, IODevice
from speculos.mcu.readerror import ReadError
from speculos.mcu.seproxyhal import SeProxyHal
from speculos.observer import BroadcastInterface
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
        self._app: Flask
        # self.sock is used by Screen.add_notifier. Closing self._notify_exit
        # signals it that the API is no longer running.
        self.sock: socket.socket
        self._notify_exit: socket.socket
        self.sock, self._notify_exit = socket.socketpair()
        self.api_port: int = api_port
        self._api_thread: threading.Thread

    @property
    def file(self):
        return self.sock

    def can_read(self, screen: DisplayNotifier) -> None:
        # Being able to read from the socket only happens when the API server exited.
        raise ReadError("API server exited")

    def _run(self) -> None:
        try:
            # threaded must be set to allow serving requests along events streaming
            self._app.run(host="0.0.0.0", port=self.api_port, threaded=True, use_reloader=False)
        except Exception as exc:
            self._app.logger.error("An exception occurred in the Flask API server: %s", exc)
            raise exc
        finally:
            self._notify_exit.close()

    def start_server_thread(self,
                            screen_: DisplayNotifier,
                            seph_: SeProxyHal,
                            automation_server: BroadcastInterface) -> None:
        wrapper = ApiWrapper(screen_, seph_, automation_server)
        self._app = wrapper.app
        self._api_thread = threading.Thread(target=self._run, name="API-server", daemon=True)
        self._api_thread.start()

    def stop(self):
        self._api_thread.join(10)


class ApiWrapper:
    def __init__(self, screen: DisplayNotifier, seph: SeProxyHal, automation_server: BroadcastInterface):

        static_folder = pkg_resources.resource_filename(__name__, "/static")
        self._app = Flask(__name__, static_url_path="", static_folder=static_folder)
        self._app.env = "development"

        screen_kwargs = {"screen": screen}
        seph_kwargs = {"seph": seph}
        app_kwargs = {"app": self._app}
        event_kwargs: Dict[str, Any] = {**app_kwargs, "automation_server": automation_server}

        self._api = Api(self.app)

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

    @property
    def app(self) -> Flask:
        return self._app
