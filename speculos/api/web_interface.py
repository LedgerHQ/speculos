from .restful import AppResource


class WebInterface(AppResource):
    def get(self):
        return self.app.send_static_file("index.html")
