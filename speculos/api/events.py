import json
import logging
import threading
from flask import stream_with_context, Response
from flask_restful import inputs, reqparse

from .restful import AppResource


class EventsBroadcaster:
    """This used to be the 'Automation Server'."""

    def __init__(self):
        self.clients = []
        self.events = []
        self.condition = threading.Condition()
        self.logger = logging.getLogger('events')

    def add_client(self, client):
        self.logger.debug("events: new client")
        self.clients.append(client)

    def remove_client(self, client):
        self.logger.debug("events: client exited")
        self.clients.remove(client)

    def broadcast(self, event):
        self.logger.debug(f"events: broadcasting {event} to ({len(self.clients)}) client(s)")
        self.events.append(event)
        for client in self.clients:
            client.send_screen_event(event)
        with self.condition:
            self.condition.notify_all()


events = EventsBroadcaster()


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
                    data = json.dumps(event)
                    # Format the event as specified in the specification:
                    # https://html.spec.whatwg.org/multipage/server-sent-events.html#parsing-an-event-stream
                    yield f"data: {data}\n\n".encode()
        finally:
            events.remove_client(self)

    def send_screen_event(self, event):
        self.events.append(event)


class Events(AppResource):
    def __init__(self, *args, **kwargs):
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("stream", type=inputs.boolean, default=False)
        super().__init__(*args, **kwargs)

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
