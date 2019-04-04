import binascii
import socket
import argparse


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Test docker connection')
    parser.add_argument('port', type=int, help='target port')
    args = parser.parse_args()

    s = socket.socket()
    s.connect(('0.0.0.0', getattr(args, 'port')))
    s.sendall(b'\x00\x00\x00\x05\xe0\xc4\x00\x00\x00')
    assert binascii.hexlify(s.recv(128)) == b'000000081b300103080100039000'
    print('Connection success!')
