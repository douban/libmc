import time
from libmc import Client


def main():
    mc = Client(['127.0.0.1:%d' % (21211 + i) for i in range(10)])
    dct = {'test_key_%d' % i: 'i' * 1000 for i in range(10)}
    while True:
        print '%.2f: set_multi: %r' % (time.time(), mc.set_multi(dct))
        print '%.2f: get_multi: %r' % (
            time.time(), mc.get_multi(dct.keys()) == dct
        )
        time.sleep(0.5)
        print


if __name__ == '__main__':
    main()
