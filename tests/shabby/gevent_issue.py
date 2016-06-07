import gevent
import time
import gevent.monkey
import slow_memcached_server
gevent.monkey.patch_time()

import greenify
greenify.greenify()
import libmc
for so_path in libmc.DYNAMIC_LIBRARIES:
    assert greenify.patch_lib(so_path)
mc = libmc.Client(["127.0.0.1:%d" % slow_memcached_server.PORT])
mc.config(libmc._client.MC_POLL_TIMEOUT,
          slow_memcached_server.BLOCKING_SECONDS * 1000 * 2)  # ms


stack = []


def mc_sleep():
    print('begin mc sleep')
    stack.append('mc_sleep_begin')
    assert mc.set('foo', 'bar')
    stack.append('mc_sleep_end')
    print('end mc sleep')


def singer():
    i = 0
    for i in range(6):
        i += 1
        print('[%d] Oh, jingle bells, jingle bells, Jingle all the way.' % i)
        stack.append('sing')
        time.sleep(0.5)


def main():
    assert len(mc.version()) == 1, "Run `python slow_memcached_server.py` first"
    mc_sleeper = gevent.spawn(mc_sleep)
    xmas_singer = gevent.spawn(singer)
    gevent.joinall([xmas_singer, mc_sleeper])
    assert stack.index('mc_sleep_end') - stack.index('mc_sleep_begin') > 1


if __name__ == '__main__':
    main()
