# coding: utf-8
import libmc
from slow_memcached_server import PORT, BLOCKING_SECONDS


def main():
    mc1 = libmc.Client(['localhost:%s' % PORT])
    mc1.config(libmc.MC_POLL_TIMEOUT, BLOCKING_SECONDS * 1000 * 2)
    assert mc1.version(), "start slow_memcached_server first"
    mc1.config(libmc.MC_POLL_TIMEOUT, BLOCKING_SECONDS * 1000 / 2)
    assert not mc1.set('foo', 1)
    mc2 = libmc.Client(['localhost:%s' % PORT])
    mc2.config(libmc.MC_POLL_TIMEOUT, BLOCKING_SECONDS * 1000 * 2)
    assert not mc1.set('foo', 1)


if __name__ == '__main__':
    main()
