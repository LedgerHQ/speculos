from flask import Flask, request, stream_with_context, Response
from flask_restful import inputs, reqparse, Resource, Api
import json
import jsonschema
import io
import logging
from PIL import Image
import pkg_resources
import threading
import time

import mcu.automation

static_folder = pkg_resources.resource_filename(__name__, "/swagger")

app = Flask(__name__, static_url_path="/", static_folder=static_folder)
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
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("action", choices=["press", "release"])
        self.parser.add_argument("delay", type=float, default=0.1)
        super(Button, self).__init__()

    def post(self):
        args = self.parser.parse_args()
        button = request.base_url.split("/")[-1]
        if button == "both":
            seph.handle_button(1, True)
            seph.handle_button(2, True)
            time.sleep(args.delay)
            seph.handle_button(1, False)
            seph.handle_button(2, False)
        else:
            buttons = { "left": 1, "right": 2 }
            actions = { "press": True, "release": False }

            assert button in buttons.keys()
            action = args.get("action", None)

            if action is None:
                seph.handle_button(buttons[button], True)
                time.sleep(args.delay)
                seph.handle_button(buttons[button], False)
            else:
                seph.handle_button(buttons[button], actions[action])

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
        self.parser.add_argument("stream", type=inputs.boolean, default=True)
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
    def __init__(self):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("x", type=int, required=True)
        self.parser.add_argument("y", type=int, required=True)
        self.parser.add_argument("action", choices=["press", "release"])
        self.parser.add_argument("delay", type=float, default=0.1)
        super(Finger, self).__init__()

    def post(self):
        args = self.parser.parse_args()
        x = args.get("x")
        y = args.get("y")
        action = args.get("action", None)

        if action is None:
            seph.handle_finger(x, y, True)
            time.sleep(args.delay)
            seph.handle_finger(x, y, False)
        else:
            seph.handle_finger(x, y, actions[action])

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
