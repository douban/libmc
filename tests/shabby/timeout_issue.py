import time
import libmc
import slow_memcached_server

def main():
    print('libmc path: %s' % libmc.__file__)
    mc = libmc.Client(['localhost:%s' % slow_memcached_server.PORT])
    mc.config(libmc.MC_POLL_TIMEOUT, 3000)
    assert mc.version(), "start slow_memcached_server first"
    EXPECTED_TIMEOUT = 300
    mc.config(libmc.MC_POLL_TIMEOUT, EXPECTED_TIMEOUT)  # timeout in 300 ms

    t0 = time.time()
    assert not mc.set('foo', 1)
    t1 = time.time()
    error_threshold = 10
    assert abs(EXPECTED_TIMEOUT - (t1 - t0) * 1000) < error_threshold


if __name__ == '__main__':
    main()
