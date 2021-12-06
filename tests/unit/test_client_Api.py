from unittest import TestCase

from speculos.client import Api


class TestApi(TestCase):

    def setUp(self):
        self.api_url = 'some random url'
        self.api = Api(self.api_url)

    def test_close_stream_None_should_not_raise(self):
        self.assertIsNone(self.api.stream)
        self.api.close_stream()
