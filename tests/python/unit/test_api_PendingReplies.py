import threading
from unittest import TestCase

from flask import Flask
from flask_restful import Api

from speculos.api.apdu import APDU, PendingReplies, pending_replies


class FakeSeph:
    """Answers every APDU at once."""

    def __init__(self, answer: bytes):
        self.apdu_callbacks = []
        self._answer = answer

    def get_tick_count(self) -> int:
        return 0

    def to_app(self, data: bytes) -> None:
        for callback in self.apdu_callbacks:
            callback(self._answer)


class TestPendingReplies(TestCase):
    def test_idle_when_nothing_pending(self):
        self.assertTrue(PendingReplies().wait_idle(0))

    def test_not_idle_until_done(self):
        pending = PendingReplies()
        pending.add()
        self.assertFalse(pending.wait_idle(0.01))
        pending.done()
        self.assertTrue(pending.wait_idle(0))

    def test_wait_wakes_up_on_done(self):
        pending = PendingReplies()
        pending.add()
        timer = threading.Timer(0.05, pending.done)
        timer.start()
        self.assertTrue(pending.wait_idle(5))
        timer.join()


class TestApduPendingReplies(TestCase):
    def setUp(self):
        self.pending = pending_replies
        app = Flask(__name__)
        api = Api(app)
        api.add_resource(APDU, "/apdu", resource_class_kwargs={"seph": FakeSeph(b"\x6d\x00")})
        self.client = app.test_client()

    def test_answer_stays_pending_until_the_response_is_closed(self):
        response = self.client.post("/apdu", json={"data": "e0ff000000"}, buffered=False)
        self.assertFalse(self.pending.wait_idle(0))
        self.assertEqual(response.get_json(), {"data": "6d00"})
        response.close()
        self.assertTrue(self.pending.wait_idle(0))

    def test_invalid_request_is_not_counted(self):
        response = self.client.post("/apdu", json={"nodata": ""})
        self.assertEqual(response.status_code, 400)
        self.assertTrue(self.pending.wait_idle(0))
