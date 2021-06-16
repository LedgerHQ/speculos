from flask import Flask, request, stream_with_context, Response
from flask_restful import inputs, reqparse, Resource, Api
import json
import jsonschema
import io
import os.path
from PIL import Image
import pkg_resources
import threading
import time

import mcu.automation

static_folder = pkg_resources.resource_filename(__name__, "/swagger")

app = Flask(__name__, static_url_path="", static_folder=static_folder)
api = Api(app)
automation_events = []


class Events:
    """This used to be the 'Automation Server'."""

    def __init__(self):
        self.clients = []
        self.events = []
        self.condition = threading.Condition()

    def add_client(self, client):
        app.logger.debug(f"events: new client")
        self.clients.append(client)

    def remove_client(self, client):
        app.logger.debug(f"events: client exited")
        self.clients.remove(client)

    def broadcast(self, event):
        app.logger.debug(f"events: broadcasting {event} to ({len(self.clients)}) client(s)")
        self.events.append(event)
        for client in self.clients:
            client.send_screen_event(event)
        with self.condition:
            self.condition.notify_all()


events = Events()


def create_app(screen_, seph_):
    global screen, seph
    screen, seph = screen_, seph_
    return app


def load_json_schema(filename):
    path = os.path.join("resources", filename)
    with pkg_resources.resource_stream(__name__, path) as fp:
        schema = json.load(fp)
    return schema


class Automation(Resource):
    def post(self):
        document = request.get_data()

        try:
            document = document.decode("ascii")
        except UnicodeDecodeError:
            return "invalid encoding", 400

        if document.startswith("file:"):
            return "invalid document", 400

        try:
            rules = mcu.automation.Automation(document)
        except json.decoder.JSONDecodeError:
            return "invalid document", 400
        except jsonschema.exceptions.ValidationError:
            return "invalid document", 400

        seph.automation = rules

        return {}, 200


class Button(Resource):
    schema = load_json_schema("button.schema")

    def post(self):
        args = request.get_json()
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        button = request.base_url.split("/")[-1]
        buttons = {"left": [1], "right": [2], "both": [1, 2]}
        action = args["action"]
        actions = {"press": [True], "release": [False], "press-and-release": [True, False]}
        delay = args.get("delay", 0.1)

        for a in actions[action]:
            for b in buttons[button]:
                seph.handle_button(b, a)
            if action == "press-and-release":
                time.sleep(delay)

        return {}, 200


class EventClient:
    def __init__(self):
        self.events = []

    def generate(self):
        try:
            # force headers to be sent
            yield b""

            while True:
                with events.condition:
                    events.condition.wait(1)

                while self.events:
                    event = self.events.pop(0)
                    data = json.dumps(event).encode()
                    yield data + b"\n"
        finally:
            events.remove_client(self)

    def send_screen_event(self, event):
        self.events.append(event)


class Events(Resource):
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("stream", type=inputs.boolean, default=False)
        super(Events, self).__init__()

    def get(self):
        args = self.parser.parse_args()
        if args.stream:
            client = EventClient()
            events.add_client(client)
            return Response(stream_with_context(client.generate()), content_type="text/event-stream")
        else:
            return {"events": events.events}, 200

    def delete(self):
        events.events.clear()
        return {}, 200


class Finger(Resource):
    schema = load_json_schema("finger.schema")

    def post(self):
        args = request.get_json()
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        action = args["action"]
        actions = {"press": [True], "release": [False], "press-and-release": [True, False]}
        delay = args.get("delay", 0.1)

        for a in actions[action]:
            seph.handle_finger(args["x"], args["y"], a)
            if action == "press-and-release":
                time.sleep(delay)

        return {}, 200


class Screenshot(Resource):
    def get(self):
        screen_size, data = screen.m.take_screenshot()
        image = Image.frombytes("RGB", screen_size, data)
        iobytes = io.BytesIO()
        image.save(iobytes, format="PNG")
        return Response(iobytes.getvalue(), mimetype="image/png")


class Swagger(Resource):
    def get(self):
        return app.send_static_file("index.html")


api.add_resource(Automation, "/automation")
api.add_resource(Button, "/button/left", "/button/right", "/button/both")
api.add_resource(Events, "/events")
api.add_resource(Finger, "/finger")
api.add_resource(Screenshot, "/screenshot")
api.add_resource(Swagger, "/")
