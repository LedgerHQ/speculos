from unittest import TestCase

from speculos.client import SpeculosClient


class TestSpeculosClient(TestCase):

    def setUp(self):
        self.app = 'app'
        self.args = ['some', 'arguments']
        self.client = SpeculosClient(self.app, self.args)

    def test_stop_successive_should_not_raise(self):
        self.assertIsNone(self.client.stream)
        self.client.stop()

    def test_stop_successive_should_not_raise2(self):
        self.client.start = lambda: None
        with self.client:
            self.client.stop()
