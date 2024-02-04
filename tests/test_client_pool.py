# coding: utf-8
import unittest
import threading
import functools
import os
from libmc import ClientPool, ThreadedClient, MC_MAX_CLIENTS

def setup_loging(f):
    g = None

    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        return g(*args, **kwargs)

    @functools.wraps(f)
    def begin(*args, **kwargs):
        nonlocal g
        with open("/tmp/debug.log", "w+") as fp:
            fp.write("")
        g = f
        return wrapper(*args, **kwargs)

    g = begin
    return wrapper

@functools.wraps(print)
@setup_loging
def threaded_print(*args, **kwargs):
    with open('/tmp/debug.log', 'a+') as fp:
        print(*args, **kwargs, file=fp)

class ClientOps:
    nthreads=8
    ops = 100

    def tid(self, mc):
        return (os.getpid(), threading.current_thread().native_id)

    def client_misc(self, mc, i=0):
        tid = self.tid(mc) + (i,)
        tid = "_".join(map(str, tid))
        f, t = 'foo_' + tid, 'tuiche_' + tid
        mc.get_multi([f, t])
        mc.delete(f)
        mc.delete(t)
        assert mc.get(f) is None
        assert mc.get(t) is None

        mc.set(f, 'biu')
        mc.set(t, 'bb')
        assert mc.get(f) == 'biu'
        assert mc.get(t) == 'bb'
        assert (mc.get_multi([f, t]) ==
                {f: 'biu', t: 'bb'})
        mc.set_multi({f: 1024, t: '8964'})
        assert (mc.get_multi([f, t]) ==
                {f: 1024, t: '8964'})

    def client_threads(self, target):
        errs = []
        def passthrough(args):
            _, e, tb, t = args
            if hasattr(e, "add_note"):
                e.add_note("Occurred in thread " + str(t))
            errs.append(e.with_traceback(tb))

        threading.excepthook = passthrough
        ts = [threading.Thread(target=target) for i in range(self.nthreads)]
        for t in ts:
            t.start()

        for t in ts:
            t.join()

        if errs:
            e = errs[0]
            if hasattr(e, "add_note"):
                e.add_note(f"Along with {len(errs)} errors in other threads")
            raise e

class ThreadedSingleServerCase(unittest.TestCase, ClientOps):
    def setUp(self):
        self.pool = ClientPool(["127.0.0.1:21211"])

    def misc(self):
        for i in range(self.ops):
            self.test_pool_client_misc(i)

    def test_pool_client_misc(self, i=0):
        with self.pool.client() as mc:
            self.client_misc(mc, i)

    def test_acquire(self):
        with self.pool.client() as mc:
            pass

    def test_pool_client_threaded(self):
        self.client_threads(self.misc)

class ThreadedClientOps(ClientOps):
    def misc(self):
        for i in range(self.ops):
            self.client_misc(self.imp, i)


class ThreadedClientWrapperCheck(unittest.TestCase, ThreadedClientOps):
    def setUp(self):
        self.imp = ThreadedClient(["127.0.0.1:21211"])

    def test_many_threads(self):
        self.client_threads(self.misc)

class ThreadedGreenletCompat(unittest.TestCase, ThreadedClientOps):
    def setUp(self):
        global gevent
        import gevent
        # import gevent.monkey
        # gevent.monkey.patch_all()

        import greenify, libmc
        greenify.greenify()
        for so_path in libmc.DYNAMIC_LIBRARIES:
            assert greenify.patch_lib(so_path)

        self.imp = libmc.ThreadedClient(["127.0.0.1:21211"])
        self.imp.config(MC_MAX_CLIENTS, 1)

    def client_threads(self, target):
        ts = [gevent.spawn(target) for i in range(self.nthreads)]
        gevent.joinall(ts, raise_error=True)

    def tid(self, mc):
        return (os.getpid(), gevent.getcurrent().name)

    def test_many_eventlets(self):
        self.client_threads(self.misc)
