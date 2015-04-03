import libmc
print libmc.__file__

# server pending = 0.5

UNIT_PENDING_SECONDS = 500  # ms

def main():
    # NOTE start 2 slow memcached server first!
    mc = libmc.Client(['localhost:8965', 'localhost:8966'])
    mc.config(libmc.MC_POLL_TIMEOUT, int(UNIT_PENDING_SECONDS * 0.5))

    bv = '1' * 512
    assert not mc.set('bv', bv)
    assert mc.get('bv') is None

    assert not mc.set('foo', 1)
    assert mc.get('stubs') is None

    mc.config(libmc.MC_POLL_TIMEOUT, int(UNIT_PENDING_SECONDS * 1.2))
    assert mc.set('foo', 1)
    assert mc.get('stubs') == 'yes'

    assert mc.set('bv', bv)
    assert mc.get('bv') is None  # this should be timed out

    assert mc.get_multi(['stubs']) == {'stubs': 'yes'}
    assert mc.get_multi(['stubs', 'foo']) == {'stubs': 'yes', 'foo': 1}
    assert not mc.get_multi(['bv'])  # this should be timed out

    # NOTE: A C++ backtrace will be printed to stderr if reproduced
    assert mc.get_multi(['bv', 'stubs']) == {'stubs': 'yes'}  # this should be timed out

    assert mc.set('foo', 1)
    assert mc.get_multi(['stubs', 'foo']) == {'stubs': 'yes', 'foo': 1}


if __name__ == '__main__':
    main()
