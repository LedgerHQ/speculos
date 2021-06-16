#!/usr/bin/env python3

'''
Quick and dirty code to exchange APDUs between Ledger Live Speculos APDU server.

Usage example:

  export DEBUG_COMM_HTTP_PROXY='http://127.0.0.1:9998'
  ledger-live getAddress -c btc --path "m/49'/0'/0'/0/0" --derivationMode segwit
'''

import argparse
import binascii
import json
import http.server
import socketserver
import socket
import sys

HOST = '127.0.0.1'
PORT = 9998


def _recvall(s, size):
    data = b''
    while size > 0:
        tmp = s.recv(size)
        if len(tmp) == 0:
            print("[-] apduthread: connection with client closed", file=sys.stderr)
            return None
        data += tmp
        size -= len(tmp)
    return data


class SimpleHTTPRequestHandler(http.server.BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", self.headers["Origin"])
        self.send_header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept")
        self.send_header("Access-Control-Allow-Methods", "GET, POST")
        self.end_headers()

    def do_POST(self):
        # get APDU from the request body
        content_length = int(self.headers['Content-Length'])
        body = self.rfile.read(content_length)
        apdu = json.loads(body)
        apdu = binascii.unhexlify(apdu['apduHex'])

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(('127.0.0.1', int(args.port)))

            if args.verbose:
                print('<', binascii.hexlify(apdu))

            # forward the APDU to the APDU server
            apdu = len(apdu).to_bytes(4, 'big') + apdu
            s.sendall(apdu)

            # receive the APDU server response
            data = _recvall(s, 4)
            size = int.from_bytes(data, byteorder='big')
            packet = _recvall(s, size + 2)

            if args.verbose:
                print('>', binascii.hexlify(packet))

        # forward the APDU response to the client, embedded in response body
        data = {'data': str(binascii.hexlify(packet), 'ascii'), 'error': None}
        data = json.dumps(data)
        data = data.encode('ascii')

        self.send_response(200)
        self.send_header('Content-Length', len(data))
        self.send_header('Content-Type', 'application/json')
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run one or more QEMU instances given the config files passed as '
                                                 'arguments. The -c/--config argument can be passed several times '
                                                 '(please note that the arguments order matters.)')
    parser.add_argument('-p', '--port', default=9999, help='APDU server port')
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer((HOST, PORT), SimpleHTTPRequestHandler) as httpd:
        print("serving at port", PORT)
        httpd.serve_forever()
