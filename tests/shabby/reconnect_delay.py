# coding: utf-8

import os
import time
import libmc
import slow_memcached_server
import subprocess


def memcached_server_ctl(cmd, port):
    ctl_path = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(
            os.path.abspath(__file__)
        ))),
        'misc', 'memcached_server'
    )
    print(ctl_path)
    subprocess.check_call([ctl_path, cmd, str(port)])


def test_soft_server_error():
    mc = libmc.Client(["127.0.0.1:%d" % slow_memcached_server.PORT])
    mc.config(libmc._client.MC_POLL_TIMEOUT,
              slow_memcached_server.BLOCKING_SECONDS * 1000 * 2)  # ms

    RETRY_TIMEOUT = 2
    mc.config(libmc.MC_RETRY_TIMEOUT, RETRY_TIMEOUT)

    assert mc.set('foo', 1)
    assert not mc.set(slow_memcached_server.KEY_SET_SERVER_ERROR.decode('utf8'), 1)
    assert mc.set('foo', 1)  # back to live
    time.sleep(RETRY_TIMEOUT / 2)
    assert mc.set('foo', 1)  # alive
    time.sleep(RETRY_TIMEOUT + 1)
    assert mc.set('foo', 1)  # alive


def test_hard_server_error():
    normal_port = 21211
    mc = libmc.Client(["127.0.0.1:%d" % normal_port])

    RETRY_TIMEOUT = 20
    mc.config(libmc.MC_RETRY_TIMEOUT, RETRY_TIMEOUT)

    assert mc.set('foo', 1)
    memcached_server_ctl('stop', normal_port)
    assert not mc.set('foo', 1)  # fail
    memcached_server_ctl('start', normal_port)
    assert not mc.set('foo', 1)  # still fail
    time.sleep(RETRY_TIMEOUT + 1)
    assert mc.set('foo', 1)  # back to live


def main():
    test_soft_server_error()
    test_hard_server_error()


if __name__ == '__main__':
    main()
