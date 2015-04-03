#!/usr/bin/env python

import cmemcached

for port in range(21211, 21231):
    mc = cmemcached.Client(["127.0.0.1:%d" % port])
    for s in ('foo', 'buzai', 'tuiche'):
        val = '%s %s' % (port, s)
        mc.set(s, val)
        assert mc.get(s) == val

    dct = {}.fromkeys(['foo%d' % i for i in range(1000)], 'lizhizhuangbi')
    mc.set_multi(dct)
