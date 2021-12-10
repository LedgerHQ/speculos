import io
from PIL import Image
from flask import Response

from .restful import ScreenResource


class Screenshot(ScreenResource):
    def get(self):
        screen_size, data = self.screen.m.take_screenshot()
        image = Image.frombytes("RGB", screen_size, data)
        iobytes = io.BytesIO()
        image.save(iobytes, format="PNG")
        response = Response(iobytes.getvalue(), mimetype="image/png")
        response.headers.add("Cache-control", "no-cache,no-store")
        return response
