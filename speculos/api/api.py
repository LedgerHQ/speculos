import socket
import threading
import pkg_resources
from typing import Dict, Optional
from flask import Flask
from flask_restful import Api

from ..mcu.readerror import ReadError
from ..mcu.seproxyhal import SeProxyHal
from .apdu import APDU
from .automation import Automation
from .button import Button
from .events import Events, EventsBroadcaster
from .finger import Finger
from .screenshot import Screenshot
from .swagger import Swagger
from .web_interface import WebInterface


class ApiRunner:
    """Run the Speculos API server, with a notification when it stops"""
    def __init__(self, api_port: int) -> None:
        self._app: Optional[Flask] = None
        # self.s is used by Screen.add_notifier. Closing self._notify_exit
        # signals it that the API is no longer running.
        self.s: socket.socket
        self._notify_exit: socket.socket
        self.s, self._notify_exit = socket.socketpair()
        self.api_port: int = api_port

    def can_read(self, s: int, screen) -> None:
        assert s == self.s.fileno()
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
                            screen_,
                            seph_: SeProxyHal,
                            automation_server: EventsBroadcaster) -> None:
        wrapper = ApiWrapper(screen_, seph_, automation_server)
        self._app = wrapper.app
        api_thread = threading.Thread(target=self._run, name="API-server", daemon=True)
        api_thread.start()


class ApiWrapper:
    def __init__(self, screen, seph: SeProxyHal, automation_server: EventsBroadcaster):
        self._screen = screen
        self._seph = seph
        self._set_app()

        self._api = Api(self.app)
        self._api.add_resource(APDU, "/apdu",
                               resource_class_kwargs=self._seph_kwargs)
        self._api.add_resource(Automation, "/automation",
                               resource_class_kwargs=self._seph_kwargs)
        self._api.add_resource(Button, "/button/left", "/button/right", "/button/both",
                               resource_class_kwargs=self._seph_kwargs)
        self._api.add_resource(Events, "/events",
                               resource_class_kwargs={**self._app_kwargs,
                                                      "automation_server": automation_server})
        self._api.add_resource(Finger, "/finger",
                               resource_class_kwargs=self._seph_kwargs)
        self._api.add_resource(Screenshot, "/screenshot",
                               resource_class_kwargs=self._screen_kwargs)
        self._api.add_resource(Swagger, "/swagger/",
                               resource_class_kwargs=self._app_kwargs)
        self._api.add_resource(WebInterface, "/",
                               resource_class_kwargs=self._app_kwargs)

    def _set_app(self):
        static_folder = pkg_resources.resource_filename(__name__, "/static")
        self._app = Flask(__name__, static_url_path="", static_folder=static_folder)
        self._app.env = "development"

    @property
    def _screen_kwargs(self):
        return {"screen": self.screen}

    @property
    def _app_kwargs(self) -> Dict[str, Flask]:
        return {"app": self.app}

    @property
    def _seph_kwargs(self) -> Dict[str, SeProxyHal]:
        return {"seph": self.seph}

    @property
    def app(self) -> Flask:
        return self._app

    @property
    def api(self) -> Api:
        return self._api

    @property
    def screen(self):
        return self._screen

    @property
    def seph(self) -> SeProxyHal:
        return self._seph
