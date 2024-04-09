import json
import logging
import threading
from typing import Dict, Generator, List, Optional, Tuple, Union
from dataclasses import asdict
from flask import stream_with_context, Response
from flask_restful import inputs, reqparse

from speculos.observer import BroadcastInterface, ObserverInterface, TextEvent
from .restful import AppResource


class EventsBroadcaster(BroadcastInterface):
    """This used to be the 'Automation Server'."""

    def __init__(self) -> None:
        super().__init__()
        self.screen_content: List[TextEvent] = []
        self.events: List[TextEvent] = []
        self.condition = threading.Condition()
        self.logger = logging.getLogger("events")

    def clear_events(self) -> None:
        self.logger.debug("Clearing events")
        self.screen_content = []

    def broadcast(self, event: TextEvent) -> None:
        if event.clear:
            self.clear_events()
            return

        self.logger.debug("events: broadcasting %s to %d client(s)", asdict(event), len(self.clients))
        self.screen_content.append(event)
        self.events.append(event)
        for client in self.clients:
            client.send_screen_event(event)
        with self.condition:
            self.condition.notify_all()


class EventClient(ObserverInterface):
    def __init__(self, broadcaster: EventsBroadcaster) -> None:
        self.events: List[TextEvent] = []
        self._broadcaster = broadcaster

    def generate(self) -> Generator[bytes, None, None]:
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

    def send_screen_event(self, event: TextEvent) -> None:
        self.events.append(event)


class Events(AppResource):
    def __init__(self, *args, automation_server: Optional[EventsBroadcaster] = None, **kwargs) -> None:
        if automation_server is None:
            raise RuntimeError("Argument 'automation_server' must not be None")
        self._broadcaster = automation_server
        self.parser = reqparse.RequestParser()
        self.parser.add_argument("stream", type=inputs.boolean, default=False, location='values')
        self.parser.add_argument("currentscreenonly", type=inputs.boolean, default=False, location='values')
        super().__init__(*args, **kwargs)

    def get(self) -> Union[Response, Tuple[Dict[str, List], int]]:
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

    def delete(self) -> Tuple[Dict, int]:
        self._broadcaster.events.clear()
        return {}, 200
