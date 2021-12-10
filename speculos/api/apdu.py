import json
import threading
import jsonschema
from flask import stream_with_context, Response, request
from typing import Generator, Optional

from ..mcu.seproxyhal import SeProxyHal
from .helpers import load_json_schema
from .restful import SephResource


class APDUBridge:
    def __init__(self, seph: SeProxyHal):
        # We want to be notified when APDU response is transmitted from the SE
        self.endpoint_lock = threading.Lock()
        self.response_condition = threading.Condition()
        self._seph = seph
        self._seph.apdu_callbacks.append(self.seph_apdu_callback)
        self.response: Optional[bytes]

    def exchange(self, data: bytes) -> Generator[bytes, None, None]:
        # force headers to be sent
        yield b""

        with self.endpoint_lock:  # Lock for a command/response for one client
            with self.response_condition:
                self.response = None
            self._seph.to_app(data)
            with self.response_condition:
                while self.response is None:
                    self.response_condition.wait()
            yield json.dumps({"data": self.response.hex()}).encode()

    def seph_apdu_callback(self, data: bytes):
        """
        Called by seph when data is transmitted by the SE. That data should
        be the response to a prior APDU request
        """
        with self.response_condition:
            self.response = data
            self.response_condition.notify()


class APDU(SephResource):
    schema = load_json_schema("apdu.schema")

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._bridge = APDUBridge(self.seph)

    def post(self):
        args = request.get_json(force=True)
        try:
            jsonschema.validate(instance=args, schema=self.schema)
        except jsonschema.exceptions.ValidationError as e:
            return {"error": f"{e}"}, 400

        data = bytes.fromhex(args.get("data"))
        return Response(stream_with_context(self._bridge.exchange(data)), content_type="application/json")
