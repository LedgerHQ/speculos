import threading
import socket
import time


class FingerClient(threading.Thread):
    '''
    Dumb thread connecting to the finger_tcp endpoint of Speculos to send him periodical touch events
    '''

    def __init__(self, host='0.0.0.0', port=1236):
        threading.Thread.__init__(self)
        self.host = host
        self.port = port
        self.running = False
        self.eventsLoop = ""

    def run(self):
        self.s = socket.create_connection((self.host, self.port), timeout=10.0)
        self.running = True
        while self.running:
            if self.eventsLoop:
                eventsLoop = ','.join(self.eventsLoop)
                self.s.sendall(bytes(eventsLoop, "ascii"))
            time.sleep(0.2)

    def stop(self):
        self.running = False

    def clear(self):
        self.eventsLoop = []