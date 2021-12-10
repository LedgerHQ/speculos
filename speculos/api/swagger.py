from .restful import AppResource


class Swagger(AppResource):
    def get(self):
        return self.app.send_static_file("swagger/index.html")
