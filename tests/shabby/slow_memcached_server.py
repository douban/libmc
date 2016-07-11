# coding: utf-8

import sys
import time
import socket

import socketserver as SocketServer

PORT = 0x2305
BLOCKING_SECONDS = 0.5  # seconds


DAYS30 = 3600 * 24 * 30
KEY_GET_SERVER_ERROR = b'gimme_get_server_error'
KEY_SET_SERVER_ERROR = b'gimme_set_server_error'
KEY_SERVER_DOWN = b'biubiubiu'

memcached = None


class Server(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    allow_reuse_address = True


def is_valid_key(key):
    if len(key) > 250:
        return False
    if any(c in key for c in (b' ', b'\0', b'\r', b'\n')):
        return False
    return True


class MemcachedProvider(object):

    def __init__(self):
        self._store = {
            'stubs': (0, time.time() + DAYS30, 3, 'yes')
        }

    def process_get(self, request):
        assert request.startswith(b'get ')
        request = request.rstrip()
        response = b''
        for key in request.split(b' ')[1:]:
            if key == KEY_GET_SERVER_ERROR:
                return b'SERVER_ERROR\r\n'
            if not is_valid_key(key):
                return b'CLIENT_ERROR invalid key\r\n'
            if key not in self._store:
                continue
            flags, exptime, bytes_, data_block = self._store[key]
            if time.time() > exptime:
                del self._store[key]
                continue
            response += (
                b'VALUE %s %d %d\r\n%s\r\n' % (
                    key, flags, bytes_, data_block
                )
            )
        response += b'END\r\n'

        return response

    def process_set(self, request):
        t0 = time.time()
        response = b''
        while request:
            assert request.startswith(b'set ')
            pos = request.find(b'\r\n')
            assert pos > 0
            set_meta = request[:pos]
            request = request[pos + 2:]

            key, flags, exptime, bytes_ = set_meta.split(b' ')[1:]
            if key == KEY_SERVER_DOWN:
                print("I'm shot. Dying.")
                return None

            if key == KEY_SET_SERVER_ERROR:
                return b'SERVER_ERROR\r\n'
            if not is_valid_key(key):
                response += b'CLIENT_ERROR invalid key\r\n'
                continue
            flags = int(flags)
            exptime = int(exptime)
            bytes_ = int(bytes_)
            if exptime == 0:
                exptime = DAYS30

            if exptime <= DAYS30:
                exptime += t0

            assert len(request) >= bytes_ + 2
            data_block = request[:bytes_]
            request = request[bytes_ + 2:]
            self._store[key] = flags, exptime, bytes_, data_block
            response += b'STORED\r\n'

        return response

    def process_version(self, request):
        return b'VERSION 1.0.24-shabby\r\n'

    def process_rest(self, request):
        print('unexpected request: %s' % request)
        return b'ERROR\r\n'

    def __getattr__(self, name):
        if name.startswith('process_'):
            return self.process_rest
        raise AttributeError()


class Handler(SocketServer.BaseRequestHandler):

    mcp = MemcachedProvider()

    def handle(self):
        while True:
            req = b''
            while req[-2:] != b'\r\n':
                try:
                    req += self.request.recv(8192)
                except socket.error as ex:
                    if ex.errno == 54:
                        pass
                    else:
                        raise ex

            print('> %s' % req[:-2])
            if not req.strip() or req.strip() == b'quit':
                break
            pos_space = req.find(b' ')
            if pos_space > 0:
                meth = req[:pos_space]
            else:
                meth = req.rstrip()

            res = getattr(self.mcp, 'process_%s' % meth.decode('ascii'), )(req)
            if res is None:
                memcached.shutdown()
                return
            if meth != 'version':
                time.sleep(BLOCKING_SECONDS)

            n_sent = 0
            while n_sent != len(res):
                n_sent += self.request.send(res[n_sent:n_sent+100])
                if n_sent != len(res):
                    print('sleep and send more')
                    time.sleep(BLOCKING_SECONDS * 2)

            print('< %s' % res[:-2])


def main(argv):
    global memcached
    port = PORT
    if len(argv) == 2:
        port = int(argv[1])

    memcached = Server(("", port), Handler)
    print('serve at tcp://%s:%s' % ('0.0.0.0', port))
    try:
        memcached.serve_forever()
    except KeyboardInterrupt:
        return
    finally:
        memcached.server_close()


if __name__ == '__main__':
    main(sys.argv)

