# -*- encoding:utf-8 -*-

import cmemcached
import unittest
import cPickle as pickle
import marshal
import time
import threading
import platform
import pytest

INVALID_SERVER_ADDR = '127.0.0.1:1'

skip_if_libmemcached_not_patched = pytest.mark.skipif(
    "gentoo" not in platform.platform(),  # FIXME: better way to detect if patched?
    reason="need to link with libmemcached with douban's patches")


class TestCmemcached(unittest.TestCase):

    @pytest.fixture(autouse=True)
    def setup(self, memcached):
        self.server_addr = memcached
        self.mc = cmemcached.Client([memcached], comp_threshold=1024)


        # mc=cmemcached.Client(["localhost:11999"], comp_threshold=1024)
        # self.assertEqual(mc.set_multi(values), 0)

    def testGetHost(self):
        host = self.mc.get_host_by_key("str")
        self.assertEqual(host, self.server_addr)

    def test_last_error(self):
        from cmemcached import RETURN_MEMCACHED_SUCCESS
        self.assertEqual(self.mc.set('testkey', 'hh'), True)
        self.assertEqual(self.mc.get('testkey'), 'hh')
        self.assertEqual(self.mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)
        self.assertEqual(self.mc.get('testkey1'), None)
        self.assertEqual(self.mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)
        self.assertEqual(self.mc.get_multi(['testkey']), {'testkey': 'hh'})
        self.assertEqual(self.mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)
        self.assertEqual(self.mc.get_multi(['testkey1']), {})
        self.assertEqual(self.mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)

        mc = cmemcached.Client([INVALID_SERVER_ADDR], comp_threshold=1024)
        self.assertEqual(mc.set('testkey', 'hh'), False)
        self.assertEqual(mc.get('testkey'), None)
        self.assertNotEqual(mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)
        self.assertEqual(mc.get_multi(['testkey']), {})
        self.assertNotEqual(mc.get_last_error(), RETURN_MEMCACHED_SUCCESS)


if __name__ == '__main__':
    unittest.main()
