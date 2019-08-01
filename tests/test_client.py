# coding: utf-8

import sys
import time
import pytest
import unittest
from libmc import (
    Client, encode_value, decode_value,
    MC_RETURN_OK, MC_RETURN_INVALID_KEY_ERR,
    MC_RETURN_MC_SERVER_ERR
)

from builtins import int
from past.builtins import basestring


# defined in _client.pyx
_FLAG_DOUBAN_CHUNKED = 1 << 12
_DOUBAN_CHUNK_SIZE = 1000000


class DiveMaster(object):

    def __init__(self, id_):
        self.id = id_

    def __eq__(self, other):
        return isinstance(other, self.__class__) and other.id == self.id


class MiscCase(unittest.TestCase):

    def test_encode_value(self):
        expect = (b'(\x02\x00\x00\x00s\x06\x00\x00\x00doubani\x00\x00\x00\x00',
                  32)
        data = (b'douban', 0)
        # FIXME
        vi = sys.version_info
        if vi.major == 2 and vi.micro == 7 and vi.minor < 9:
            assert encode_value(data, 0) == expect

    def test_decode_value(self):
        dataset = [
            True,
            0,
            100,
            int(1000),
            10.24,
            DiveMaster(1024),
            "scubadiving",
            b'haha',
            ('douban', 0),
        ]
        for d in dataset:
            new_d = decode_value(*encode_value(d, 0))
            assert new_d == d
            if isinstance(d, DiveMaster):
                assert d is not new_d


class SingleServerCase(unittest.TestCase):
    def setUp(self):
        self.mc = Client(["127.0.0.1:21211"])
        self.mc_alt = Client(["127.0.0.1:21211"])
        self.compressed_mc = Client(["127.0.0.1:21211"], comp_threshold=1024)
        self.noreply_mc = Client(["127.0.0.1:21211"], noreply=True)

    def test_attribute(self):
        "Test attributes are accessible from Python code"
        mc = self.mc
        assert hasattr(mc, 'comp_threshold')
        assert hasattr(mc, 'servers')

    def test_misc(self):
        mc = self.mc
        mc.get_multi(['foo', 'tuiche'])
        mc.delete('foo')
        mc.delete('tuiche')
        assert mc.get('foo') is None
        assert mc.get('tuiche') is None

        mc.set('foo', 'biu')
        mc.set('tuiche', 'bb')
        assert mc.get('foo') == 'biu'
        assert mc.get('tuiche') == 'bb'
        assert (mc.get_multi(['foo', 'tuiche']) ==
                {'foo': 'biu', 'tuiche': 'bb'})
        mc.set_multi({'foo': 1024, 'tuiche': '8964'})
        assert (mc.get_multi(['foo', 'tuiche']) ==
                {'foo': 1024, 'tuiche': '8964'})

    def test_delete(self):
        mc = self.mc
        assert mc.set('smmf', 0xCA909) is True
        assert mc.get('smmf') == 0xCA909
        assert mc.delete('smmf') is True
        assert mc.delete_multi(['smmf']) is True
        assert mc.set('smmf', 0xCA909) is True
        assert mc.delete_multi(['smmf']) is True
        assert mc.delete('smmf') is True
        assert mc.delete('smmf') is True

    def test_incr_decr(self):
        mc = self.mc
        mc.set('wazi', 99)
        assert mc.incr('wazi', 1) == 100
        assert mc.decr('wazi', 1) == 99
        mc.delete('wazi')
        assert mc.incr('wazi', 1) is None
        assert mc.decr('wazi', 1) is None

    def test_cas(self):
        mc = self.mc
        mc.delete('bilinda')
        k1 = 'bilinda'
        v1 = 'butchers'
        assert mc.gets(k1) is None
        mc.set(k1, v1)
        v1_, ck = mc.gets('bilinda')
        v2 = 'ding'
        assert v1 == v1_
        assert mc.cas(k1, v2, 0, ck) is True
        assert mc.cas(k1, v1, 0, ck) is not True
        v2_, ck = mc.gets('bilinda')
        assert v2 == v2_

    def test_large(self):
        mc = self.mc
        BUF_500KB = 'i' * (500 * 1000)
        BUF_1MB = 'i' * (1000 * 1000)
        key_500kb = 'test_500kb'
        key_1mb = 'test_1mb'
        mc.delete(key_500kb)
        mc.delete(key_1mb)
        mc.set(key_500kb, BUF_500KB)
        assert mc.get(key_500kb) == BUF_500KB
        mc.set(key_1mb, BUF_1MB)
        assert mc.get(key_1mb) == BUF_1MB
        mc.delete(key_500kb)
        mc.delete(key_1mb)
        dct = {key_500kb: BUF_500KB, key_1mb: BUF_1MB}
        mc.set_multi(dct)
        assert mc.get_multi(list(dct.keys())) == dct

    def test_extra_large(self):
        threshold = 1000000
        mc = self.mc
        key_xl = 'test_very_large'
        val = 'i' * threshold
        assert mc.set(key_xl, val)
        assert mc.get(key_xl) == val
        val += 'b'
        assert mc.set(key_xl, val)
        assert mc.get(key_xl) == val

        val = 'i' * (threshold + threshold)
        assert mc.set(key_xl, val)
        assert mc.get(key_xl) == val

        val += 'b'
        assert mc.set(key_xl, val)
        assert mc.get(key_xl) == val

    def test_noreply(self):
        mc = self.noreply_mc
        assert mc.set('foo', 'bar')
        assert mc.touch('foo', 30)
        v, ck = mc.gets('foo')
        assert mc.cas('foo', 'bar2', 0, ck)
        mc.delete('foo')
        assert mc.set('foo', 1024)
        assert mc.incr('foo', 1) is None
        assert mc.get('foo') == 1025
        assert mc.decr('foo', 3) is None
        assert mc.get('foo') == 1022

    def test_injection(self):
        # credit to The New Page of Injections Book:
        #   Memcached Injections @ blackhat2014 [pdf](http://t.cn/RP0J10Z)
        mc = self.mc
        assert mc.delete('injected')
        assert mc.set('a' * 250, 'biu')
        assert not mc.set('a' * 251, 'biu')
        mc.set('a' * 251, 'set injected 0 3600 10\r\n1234567890')
        assert mc.get('injected') is None
        mc.delete('injected')
        mc.set('key1', '1234567890')
        mc.set('key1 0', '123456789012345678901234567890\r\n'
                         'set injected 0 3600 3\r\nINJ\r\n')
        assert mc.get('injected') is None

    def test_maxiov(self):
        key_tmpl = 'not_existed.%s'
        assert self.mc.get_multi([key_tmpl % i for i in range(10000)]) == {}

    def test_get_set_raw(self):
        self.mc.set('foo', 233)
        assert self.mc.get_raw('foo') == (b'233', 2)
        self.mc.set_raw('foo', b'2335', 0, 2)
        assert self.mc.get('foo') == 2335

    def test_stats(self):
        stats = self.mc.stats()
        for addr, dct in stats.items():
            assert isinstance(dct['version'], basestring)
            assert (isinstance(dct['rusage_system'], float) or
                    isinstance(dct['rusage_user'], float))
            assert isinstance(dct['curr_connections'], int)

    def test_get_set_large_raw(self):
        key = 'large_raw_key'
        key_dup = '%s_dup' % key
        val = 'i' * int(_DOUBAN_CHUNK_SIZE * 1.5)
        for mc in (self.mc, self.compressed_mc):
            mc.set(key, val)
            assert mc.get(key) == val
            raw_val1, flags1 = mc.get_raw(key)
            assert mc.set_raw(key_dup, raw_val1, 0, flags1)
            assert mc.get_raw(key) == mc.get_raw(key_dup)

    def test_patch_no_compress(self):
        key = 'no_compress'
        val = b'1024'
        self.mc.set(key, val, compress=False)
        large_patch = b'hahahaha' * 512
        self.mc.prepend(key, large_patch)
        assert self.mc.get_raw(key)[0] == large_patch + val
        self.mc.delete(key)

        self.mc.set(key, val, compress=False)
        large_patch = b'hahahaha' * 512
        self.mc.append(key, large_patch)
        assert self.mc.get_raw(key)[0] == val + large_patch
        self.mc.delete(key)

    def test_quit(self):
        assert self.mc.delete('all_is_well')
        assert self.mc.set('all_is_well', 'bingo')
        assert self.mc_alt.delete('all_is_well')
        assert self.mc_alt.set('all_is_well', 'bingo')
        assert self.mc.version()  # establish all conns
        assert self.mc_alt.version()  # establish all conns
        nc1 = self.mc.stats()[self.mc.servers[0]]['curr_connections']
        nc2 = self.mc_alt.stats()[self.mc.servers[0]]['curr_connections']
        assert nc1 == nc2
        assert self.mc.quit()
        max_wait = 3
        while nc1 - 1 != nc2 and max_wait > 0:
            nc2 = self.mc_alt.stats()[self.mc.servers[0]]['curr_connections']
            max_wait -= 1
            time.sleep(1)
        assert nc1 - 1 == nc2
        # back to life immediately
        assert self.mc.get('all_is_well') == 'bingo'

    def test_flush_all(self):
        keys = ["flush_all_check_%s" % i for i in range(1000)]
        value = "testing_flush_all"
        dict_to_set = {
            key: value
            for key in keys
        }
        self.mc.set_multi(dict_to_set)
        retrieved_dct = self.mc.get_multi(keys)
        assert retrieved_dct == dict_to_set

        with pytest.raises(RuntimeError, match=r".*toggle.*"):
            rtn = self.mc.flush_all()
        self.mc.toggle_flush_all_feature(True)
        rtn = self.mc.flush_all()
        assert isinstance(rtn, list)
        assert rtn == self.mc.servers
        assert {} == self.mc.get_multi(keys)

        self.mc.toggle_flush_all_feature(False)
        with pytest.raises(RuntimeError, match=r".*toggle.*"):
            rtn = self.mc.flush_all()

class TwoServersCase(SingleServerCase):

    def setUp(self):
        self.mc = Client(["127.0.0.1:21211", "127.0.0.1:21212"])
        self.mc_alt = Client(["127.0.0.1:21211", "127.0.0.1:21212"])
        self.compressed_mc = Client(["127.0.0.1:21211", "127.0.0.1:21212"],
                                    comp_threshold=1024)
        self.noreply_mc = Client(
            ["127.0.0.1:21211", "127.0.0.1:21212"], noreply=True
        )


class TenServersCase(SingleServerCase):

    def setUp(self):
        self.mc = Client(["127.0.0.1:%d" % (21211 + i) for i in range(10)])
        self.mc_alt = Client(["127.0.0.1:%d" % (21211 + i) for i in range(10)])
        self.compressed_mc = Client(
            ["127.0.0.1:%d" % (21211 + i) for i in range(10)],
            comp_threshold=1024
        )
        self.noreply_mc = Client(
            ["127.0.0.1:%d" % (21211 + i) for i in range(10)], noreply=True
        )


class ErrorCodeTestCase(unittest.TestCase):

    def setUp(self):
        self.mc = Client(["127.0.0.1:21211"])
        assert self.mc.version()
        assert self.mc.get_last_error() == MC_RETURN_OK
        assert self.mc.get_last_strerror() == "ok"

    def test_invalid_key(self):
        self.mc.get('invalid key')
        assert self.mc.get_last_error() == MC_RETURN_INVALID_KEY_ERR
        assert self.mc.get_last_strerror() == "invalid_key_error"

    def test_mc_server_err(self):
        mc = Client(["not_exist_host:11211"])
        mc.get('valid_key')
        assert mc.get_last_error() == MC_RETURN_MC_SERVER_ERR
        assert mc.get_last_strerror() == "server_error"
