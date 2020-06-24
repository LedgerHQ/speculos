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
        self.state = []
        self.prev_state = []

    def add_client(self, client):
        self.clients.append(client)

    def remove_client(self, client):
        self.clients.remove(client)

    def screen_event(self, text, x, y):
        """
        Accumulates display events in internal state structure.

        This methods accumulates display events in the `state` list member.
        It assumes the screen display as a list of test lines so it keeps
        the list sorted regarding the `y` coordinate of the events.
        """
        event = { "text": text.decode("ascii", "ignore"), "x": x, "y": y }
        if event not in self.state:
            self.state.append(event)
            self.state.sort(key=lambda e: e['y'])

    def broadcast_screen_state(self):
        """
        Broadcasts screen state to all connected clients as json contents.

        This method must be called at screen refresh (`UX_REDISPLAY`).
        In return, it checks whether the current `state` differs from the
        previously broadcasted state in `prev_state` and if no changes
        occurred, no event is sent.
        """
        if self.state and self.state != self.prev_state:
            self.logger.debug(f"broadcast {self.state} to {self.clients}")
            for client in self.clients:
                client.send_screen_event(self.state)
            self.prev_state = self.state
        self.state = []

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
