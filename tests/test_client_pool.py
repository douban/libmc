# coding: utf-8
import unittest
from threading import Thread
from libmc import ClientPool


class ThreadedSingleServerCase(unittest.TestCase):
    def setUp(self):
        self.pool = ClientPool(["127.0.0.1:21211"])

    def misc(self):
        for i in range(5):
            self.test_pool_client_misc(i)

    def test_acquire(self):
        with self.pool.client() as mc:
            pass

    def test_pool_client_misc(self, i=0):
        with self.pool.client() as mc:
            tid = mc._get_current_thread_ident() + (i,)
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

    def test_pool_client_threaded(self):
        ts = [Thread(target=self.misc) for i in range(8)]
        for t in ts:
            t.start()

        for t in ts:
            t.join()
