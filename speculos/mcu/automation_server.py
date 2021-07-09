"""
Forward screen events (text, x, y) to TCP clients.
"""

import json
import logging
import socketserver
import threading


class AutomationServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, server_address, RequestHandlerClass, *args, **kwargs):
        socketserver.TCPServer.__init__(self, server_address, RequestHandlerClass)

        self.logger = logging.getLogger("automation")
        self.clients = []

    def add_client(self, client):
        self.clients.append(client)

    def remove_client(self, client):
        self.clients.remove(client)

    def broadcast(self, event):
        """Broadcast an event to each connected client."""

        self.logger.debug(f"broadcast {event} to {self.clients}")
        for client in self.clients:
            client.send_screen_event(event)


class AutomationClient(socketserver.BaseRequestHandler):
    def setup(self):
        self.logger = logging.getLogger("automation")
        self.condition = threading.Condition()
        self.events = []
        self.logger.debug(f"new client from {self.client_address}")
        self.server.add_client(self)

    def handle(self):
        while True:
            with self.condition:
                while len(self.events) == 0:
                    self.condition.wait()

            while self.events:
                event = self.events.pop(0)
                data = json.dumps(event).encode()
                try:
                    self.request.sendall(data + b"\n")
                except BrokenPipeError:
                    return

    def finish(self):
        self.logger.debug("connection closed with client")
        self.server.remove_client(self)

    def send_screen_event(self, event):
        self.events.append(event)
        with self.condition:
            self.condition.notify()
