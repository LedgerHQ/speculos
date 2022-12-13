import json
import logging
import threading
from typing import Optional, List
from dataclasses import asdict
from flask import stream_with_context, Response
from flask_restful import inputs, reqparse

from .restful import AppResource
from ..mcu.automation import TextEvent

# Approximative minimum vertical distance between two lines of text on the devices' screen.
MIN_LINES_HEIGHT_DISTANCE = 10  # pixels


class EventsBroadcaster:
    """This used to be the 'Automation Server'."""

    def __init__(self):
        self.clients = []
        self.screen_content: List[TextEvent] = []
        self.events: List[TextEvent] = []
        self.condition = threading.Condition()
        self.logger = logging.getLogger("events")

    def add_client(self, client):
        self.logger.debug("events: new client")
        self.clients.append(client)

    def remove_client(self, client):
        self.logger.debug("events: client exited")
        self.clients.remove(client)

    def broadcast(self, event: TextEvent):
        self.logger.debug(f"events: broadcasting {asdict(event)} to ({len(self.clients)}) client(s)")
        if self.screen_content:
            # Reset screen content if new event is not below the last text line of
            # current screen. Event Y coordinate starts at the top of the screen.
            if event.y <= self.screen_content[-1].y + MIN_LINES_HEIGHT_DISTANCE:
                self.screen_content = []
        self.screen_content.append(event)
        self.events.append(event)
        for client in self.clients:
            client.send_screen_event(event)
        with self.condition:
            self.condition.notify_all()


class EventClient:
    def __init__(self, broadcaster: EventsBroadcaster):
        self.events: List[TextEvent] = []
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
                    data = json.dumps(asdict(event))
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
        self.parser.add_argument("currentscreenonly", type=inputs.boolean, default=False, location='values')
        super().__init__(*args, **kwargs)

    def get(self):
        args = self.parser.parse_args()
        if args.stream:
            client = EventClient(self._broadcaster)
            self._broadcaster.add_client(client)
            return Response(stream_with_context(client.generate()), content_type="text/event-stream")
        elif args.currentscreenonly:
            event_list = self._broadcaster.screen_content
        else:
            event_list = self._broadcaster.events
        return {"events": [asdict(e) for e in event_list]}, 200

    def delete(self):
        self._broadcaster.events.clear()
        return {}, 200
