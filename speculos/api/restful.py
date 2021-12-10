from flask import Flask
from typing import Optional
from flask_restful import Resource

from ..mcu.seproxyhal import SeProxyHal


class SephResource(Resource):
    def __init__(self, *args, seph: Optional[SeProxyHal] = None, **kwargs):
        if seph is None:
            raise RuntimeError("Argument 'seph' must not be None")
        super().__init__(*args, **kwargs)
        self._seph = seph

    @property
    def seph(self):
        return self._seph


class AppResource(Resource):
    def __init__(self, *args, app: Optional[Flask] = None, **kwargs):
        if app is None:
            raise RuntimeError("Argument 'app' must not be None")
        super().__init__(*args, **kwargs)
        self._app = app

    @property
    def app(self):
        return self._app


class ScreenResource(Resource):
    def __init__(self, *args, screen=None, **kwargs):
        if screen is None:
            raise RuntimeError("Argument 'screen' must not be None")
        super().__init__(*args, **kwargs)
        self._screen = screen

    @property
    def screen(self):
        return self._screen
