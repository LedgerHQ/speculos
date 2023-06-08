from flask import Response

from .restful import ScreenResource


class Screenshot(ScreenResource):
    def get(self):
        iobytes_value = self.screen.m.take_published_screenshot()
        response = Response(iobytes_value, mimetype="image/png")
        response.headers.add("Cache-control", "no-cache,no-store")
        return response
