# if GIL is released before sleep in C++, the following output is expected:
#
# $ PYTHONPATH=../../ python gil_issue.py
# sleep in C++ for 5s, you can sing infinitely now, singer.
# [1] Oh, jingle bells, jingle bells, Jingle all the way.
# [2] Oh, jingle bells, jingle bells, Jingle all the way.
# [3] Oh, jingle bells, jingle bells, Jingle all the way.
# [4] Oh, jingle bells, jingle bells, Jingle all the way.
# [5] Oh, jingle bells, jingle bells, Jingle all the way.
# exit sleep
# [6] Oh, jingle bells, jingle bells, Jingle all the way.
# [7] Oh, jingle bells, jingle bells, Jingle all the way.
# ^C

import time
import threading
from libmc import Client

stack = []


def singer():
    i = 0
    while i < 6:
        i += 1
        print('[%d] Oh, jingle bells, jingle bells, Jingle all the way.' % i)
        stack.append('sing')
        time.sleep(0.5)


def mc_sleeper():
    s = 2
    print('sleep in C++ for %ds, you can sing infinitely now, singer.' % s)
    mc = Client(["127.0.0.1:21211"])
    release_gil = True  # toggle here
    stack.append('mc_sleep_begin')
    mc._sleep(s, release_gil)
    stack.append('mc_sleep_end')
    print('exit sleep')


def main():
    t = threading.Thread(target=singer)
    t.start()
    mc_sleeper()
    t.join()
    assert stack.index('mc_sleep_end') - stack.index('mc_sleep_begin') > 1


if __name__ == '__main__':
    main()
