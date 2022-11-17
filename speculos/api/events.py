import json
import logging
import threading
from typing import Optional
from flask import stream_with_context, Response
from flask_restful import inputs, reqparse

from .restful import AppResource


class EventsBroadcaster:
    """This used to be the 'Automation Server'."""

    def __init__(self):
        self.clients = []
        self.screen_content = []
        self.events = []
        self.condition = threading.Condition()
        self.logger = logging.getLogger("events")

    def add_client(self, client):
        self.logger.debug("events: new client")
        self.clients.append(client)

    def remove_client(self, client):
        self.logger.debug("events: client exited")
        self.clients.remove(client)

    def broadcast(self, event):
        self.logger.debug(f"events: broadcasting {event} to ({len(self.clients)}) client(s)")
        if self.screen_content:
            y_trigger = self.screen_content[-1]["y"] + 10
            if event["y"] <= y_trigger:
                # Reset screen content
                self.screen_content = []
        self.screen_content.append(event)
        self.events.append(event)
        for client in self.clients:
            client.send_screen_event(event)
        with self.condition:
            self.condition.notify_all()


class EventClient:
    def __init__(self, broadcaster: EventsBroadcaster):
        self.events = []
        self._broadcaster = broadcaster

    def generate(self):
        try:
            # force headers to be sent
            yield b""

            while True:
                with self._broadcaster.condition:
                    self._broadcaster.condition.wait(1)

                while self.events:
                    event = self.events.pop(0)
                    data = json.dumps(event)
                    # Format the event as specified in the specification:
                    # https://html.spec.whatwg.org/multipage/server-sent-events.html#parsing-an-event-stream
                    yield f"data: {data}\n\n".encode()
        finally:
            self._broadcaster.remove_client(self)

    def send_screen_event(self, event):
        self.events.append(event)


class Events(AppResource):
    def __init__(self, *args, automation_server: Optional[EventsBroadcaster] = None, **kwargs):
        if automation_server is None:
            raise RuntimeError("Argument 'automation_server' must not be None")
        self._broadcaster = automation_server
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("stream", type=inputs.boolean, default=False, location='values')
        self.parser.add_argument("screencontent", type=inputs.boolean, default=False, location='values')
        super().__init__(*args, **kwargs)

    def get(self):
        args = self.parser.parse_args()
        if args.stream:
            client = EventClient(self._broadcaster)
            self._broadcaster.add_client(client)
            return Response(stream_with_context(client.generate()), content_type="text/event-stream")
        elif args.screencontent:
            return {"events": self._broadcaster.screen_content}, 200
        else:
            return {"events": self._broadcaster.events}, 200

    def delete(self):
        self._broadcaster.events.clear()
        return {}, 200
