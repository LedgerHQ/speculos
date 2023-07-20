"""
Forward screen events (text, x, y) to TCP clients.
"""

import json
import logging
import socketserver
import threading
from typing import List
from dataclasses import asdict

from speculos.observer import BroadcastInterface, ObserverInterface, TextEvent


class AutomationServer(socketserver.ThreadingMixIn, socketserver.TCPServer, BroadcastInterface):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, server_address, RequestHandlerClass, *args, **kwargs):
        BroadcastInterface.__init__(self)
        socketserver.TCPServer.__init__(self, server_address, RequestHandlerClass)
        self.logger = logging.getLogger("automation")

    def broadcast(self, event: TextEvent):
        """Broadcast an event to each connected client."""

        self.logger.debug("Broadcast %s to %s", asdict(event), self.clients)
        for client in self.clients:
            client.send_screen_event(event)


class AutomationClient(socketserver.BaseRequestHandler, ObserverInterface):

    def setup(self) -> None:
        self.server:  AutomationServer
        self.logger = logging.getLogger("automation")
        self.condition = threading.Condition()
        self.events: List[TextEvent] = []
        self.logger.debug("New client from %s", self.client_address)
        self.server.add_client(self)

    def handle(self):
        while True:
            with self.condition:
                while len(self.events) == 0:
                    self.condition.wait()

            while self.events:
                event = self.events.pop(0)
                data = json.dumps(asdict(event)).encode()
                try:
                    self.request.sendall(data + b"\n")
                except BrokenPipeError:
                    return

    def finish(self):
        self.logger.debug("Connection closed with client %s", self)
        self.server.remove_client(self)

    def send_screen_event(self, event: TextEvent) -> None:
        self.events.append(event)
        with self.condition:
            self.condition.notify()
