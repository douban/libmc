# coding: utf-8
import time
import threading
import warnings

from libmc import Client, ThreadUnsafe
import unittest
import pytest

from builtins import int

# defined in _client.pyx
_FLAG_DOUBAN_CHUNKED = 1 << 12
_DOUBAN_CHUNK_SIZE = 1000000


try:
    import cmemcached
except ImportError:
    cmemcached = None

try:
    import numpy as np
except ImportError:
    np = None


class BigObject(object):

    def __init__(self, letter='1', size=2000000):
        self.object = letter * size

    def __eq__(self, other):
        return self.object == other.object


class NoPickle(object):

    def __getattr__(self, name):
        pass


class CmemcachedRegressionCase(unittest.TestCase):

    def setUp(self):
        host = "127.0.0.1"
        port = 21211
        self.server_addr = '%s:%d' % (host, port)
        self.mc = Client([self.server_addr], comp_threshold=1024)
        if cmemcached is not None:
            self.old_mc = cmemcached.Client([self.server_addr],
                                            comp_threshold=1024)

    def test_set_get(self):
        self.mc.set("key", "value")
        self.assertEqual(self.mc.get("key"), "value")

        self.mc.set("key_int", 1)
        self.assertEqual(self.mc.get("key_int"), True)

        self.mc.set("key_long", int(1234567890))
        self.assertEqual(self.mc.get("key_long"), int(1234567890))

        self.mc.set("key_object", BigObject())
        self.assertEqual(self.mc.get("key_object"), BigObject())

        big_object = BigObject('x', 1000001)
        self.mc.set("key_big_object", big_object)
        self.assertEqual(self.mc.get("key_big_object"), big_object)

    def test_set_get_none(self):
        self.assertEqual(self.mc.set('key', None), True)
        self.assertEqual(self.mc.get('key'), None)

    def test_chinese_set_get(self):
        key = '豆瓣'
        value = '在炎热的夏天我们无法停止上豆瓣'
        self.assertEqual(self.mc.set(key, value), True)

        self.assertEqual(self.mc.get(key), value)

    def test_unicode_set_get(self):
        key = "test_unicode_set_get"
        value = u"中文"
        self.assertEqual(self.mc.set(key, value), True)
        self.assertEqual(self.mc.get(key), value)

    def test_special_key(self):
        key = 'keke a kid'
        value = 1024
        self.assertEqual(self.mc.set(key, value), False)
        self.assertEqual(self.mc.get(key), None)
        key = 'u:keke a kid'
        self.assertEqual(self.mc.set(key, value), False)
        self.assertEqual(self.mc.get(key), None)

    def test_unicode_key(self):
        key1 = u"answer"
        key2 = u"答案"
        bytes_key1 = "answer"
        bytes_key2 = "答案"
        value = 42

        self.assertEqual(self.mc.set(key1, value), True)
        self.assertEqual(self.mc.get(key1), value)

        self.assertEqual(self.mc.set(key2, value), True)
        self.assertEqual(self.mc.get(key2), value)

        self.assertEqual(self.mc.incr(key2), value + 1)
        self.assertEqual(self.mc.get(key2), value + 1)

        self.assertEqual(self.mc.delete(key1), True)
        self.assertEqual(self.mc.get(key1), None)

        self.assertEqual(self.mc.add(key1, value), True)
        self.assertEqual(self.mc.get(key1), value)
        self.assertEqual(self.mc.add(key1, value), False)
        self.assertEqual(self.mc.set(key1, value), True)

        self.assertEqual(self.mc.get(bytes_key1), self.mc.get(key1))
        self.assertEqual(self.mc.get(bytes_key2), self.mc.get(key2))

    def test_add(self):
        key = 'test_add'
        self.mc.delete(key)
        self.assertEqual(self.mc.add(key, 'tt'), True)
        self.assertEqual(self.mc.get(key), 'tt')
        self.assertEqual(self.mc.add(key, 'tt'), 0)
        self.mc.delete(key + '2')
        self.assertEqual(self.mc.add(key + '2', range(10)), True)

    def test_replace(self):
        key = 'test_replace'
        self.mc.delete(key)
        self.assertEqual(self.mc.replace(key, ''), 0)
        self.assertEqual(self.mc.set(key, 'b'), True)
        self.assertEqual(self.mc.replace(key, 'a'), True)
        self.assertEqual(self.mc.get(key), 'a')

    def test_append(self):
        key = "test_append"
        value = b"append\n"
        self.mc.delete(key)
        self.assertEqual(self.mc.append(key, value), 0)
        self.mc.set(key, b"")
        self.assertEqual(self.mc.append(key, value), True)
        self.assertEqual(self.mc.append(key, value), True)
        self.assertEqual(self.mc.prepend(key, b'before\n'), True)
        self.assertEqual(self.mc.get(key), b'before\n' + value * 2)

    def test_set_multi(self):
        values = dict(('key%s' % k, ('value%s' % k) * 100)
                      for k in range(1000))
        values.update({' ': ''})
        self.assertEqual(self.mc.set_multi(values), False)
        del values[' ']
        self.assertEqual(self.mc.get_multi(list(values.keys())), values)

    def test_append_large(self):
        k = 'test_append_large'
        self.mc.set(k, b'a' * 2048)
        self.mc.append(k, b'bbbb')
        assert b'bbbb' not in self.mc.get(k)
        self.mc.set(k, b'a' * 2048, compress=False)
        self.mc.append(k, b'bbbb')
        assert b'bbbb' in self.mc.get(k)

    def test_incr(self):
        key = "Not_Exist"
        self.assertEqual(self.mc.incr(key), None)
        # key="incr:key1"
        # self.mc.set(key, "not_numerical")
        # self.assertEqual(self.mc.incr(key), 0)
        key = "incr:key2"
        self.mc.set(key, 2007)
        self.assertEqual(self.mc.incr(key), 2008)

    def test_decr(self):
        key = "Not_Exist"
        self.assertEqual(self.mc.decr(key), None)
        # key="decr:key1"
        # self.mc.set(key, "not_numerical")
        # self.assertEqual(self.mc.decr(key),0)
        key = "decr:key2"
        self.mc.set(key, 2009)
        self.assertEqual(self.mc.decr(key), 2008)

    def test_get_multi(self):
        keys = ["hello1", "hello2", "hello3"]
        values = ["vhello1", "vhello2", "vhello3"]
        for x in range(3):
            self.mc.set(keys[x], values[x])
            self.assertEqual(self.mc.get(keys[x]), values[x])
        result = self.mc.get_multi(keys)
        for x in range(3):
            self.assertEqual(result[keys[x]], values[x])

    def test_get_multi_invalid(self):
        keys = ["hello1", "hello2", "hello3"]
        values = ["vhello1", "vhello2", "vhello3"]
        for x in range(3):
            self.mc.set(keys[x], values[x])
            self.assertEqual(self.mc.get(keys[x]), values[x])
        invalid_keys = keys + ['hoho\r\n']
        result = self.mc.get_multi(invalid_keys)
        for x in range(3):
            self.assertEqual(result[keys[x]], values[x])
        result_new = self.mc.get_multi(keys)
        for x in range(3):
            self.assertEqual(result_new[keys[x]], values[x])

    def test_get_multi_big(self):
        keys = ["hello1", "hello2", "hello3"]
        values = [BigObject(str(i), 1000001) for i in range(3)]
        for x in range(3):
            self.mc.set(keys[x], values[x])
            self.assertEqual(self.mc.get(keys[x]), values[x])
        result = self.mc.get_multi(keys)
        for x in range(3):
            self.assertEqual(result[keys[x]], values[x])

    def test_get_multi_with_empty_string(self):
        keys = ["hello1", "hello2", "hello3"]
        for k in keys:
            self.mc.set(k, '')
        self.assertEqual(self.mc.get_multi(keys), dict(zip(keys, [""] * 3)))

    def test_bool(self):
        self.mc.set("bool", True)
        value = self.mc.get("bool")
        self.assertEqual(value, True)
        self.mc.set("bool_", False)
        value = self.mc.get("bool_")
        self.assertEqual(value, False)

    def testEmptyString(self):
        self.assertTrue(self.mc.set("str", ''))
        value = self.mc.get("str")
        self.assertEqual(value, '')

    def test_pickle(self):
        v = [{"v": BigObject('a', 10)}]
        self.mc.set("a", v)
        self.assertEqual(self.mc.get("a"), v)
        # TODO
        '''
        raw, flags = self.mc.get_raw("a")
        self.assertEqual(raw, pickle.dumps(v, -1))
        '''

    def test_no_pickle(self):
        v = NoPickle()
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            self.assertEqual(self.mc.set("nopickle", v), None)
        self.assertEqual(self.mc.get("nopickle"), None)

    def test_marshal(self):
        v = [{2: {"a": 337}}]
        self.mc.set("a", v)
        self.assertEqual(self.mc.get("a"), v)
        # TODO
        '''
        raw, flags = self.mc.get_raw("a")
        self.assertEqual(raw, marshal.dumps(v, 2))
        '''

    def test_big_list(self):
        v = range(1024 * 1024)
        r = self.mc.set('big_list', v)

        self.assertEqual(r, True)
        self.assertEqual(self.mc.get('big_list'), v)

    def test_touch(self):
        self.mc.set('test', True)
        self.assertEqual(self.mc.get('test'), True)
        self.assertEqual(self.mc.touch('test', -1), True)
        self.assertEqual(self.mc.get('test'), None)

        self.mc.set('test', True)
        self.assertEqual(self.mc.get('test'), True)
        self.assertEqual(self.mc.touch('test', 1), True)
        time.sleep(1)
        self.assertEqual(self.mc.get('test'), None)

    def test_client_pickable(self):
        import pickle
        d = pickle.dumps(self.mc)
        self.mc = pickle.loads(d)
        self.test_stats()

    def test_stats(self):
        s = self.mc.stats()
        self.assertEqual(self.server_addr in s, True)
        st = s[self.server_addr]
        st_keys = {
            "pid",
            "uptime",
            "time",
            "version",
            "pointer_size",
            "rusage_user",
            "rusage_system",
            "curr_items",
            "total_items",
            "bytes",
            "curr_connections",
            "total_connections",
            "connection_structures",
            "cmd_get",
            "cmd_set",
            "get_hits",
            "get_misses",
            "evictions",
            "bytes_read",
            "bytes_written",
            "limit_maxbytes",
            "threads",
        }
        self.assertTrue(set(st.keys()) >= st_keys)
        ''' TODO
        mc = cmemcached.Client([INVALID_SERVER_ADDR, self.server_addr])
        s = mc.stats()
        self.assertEqual(len(s), 2)
        '''

    def test_append_multi(self):
        N = 10
        K = "test_append_multi_%d"
        data = b"after\n"
        for i in range(N):
            self.assertEqual(self.mc.set(K % i, b"before\n"), True)
        keys = [K % i for i in range(N)]
        # append
        self.assertEqual(self.mc.append_multi(keys, data), True)
        self.assertEqual(self.mc.get_multi(keys),
                         dict(zip(keys, [b"before\n" + data] * N)))
        # prepend
        self.assertEqual(self.mc.prepend_multi(keys, data), True)
        self.assertEqual(self.mc.get_multi(keys),
                         dict(zip(keys, [data + b"before\n" + data] * N)))
        # delete
        self.assertEqual(self.mc.delete_multi(keys), True)
        self.assertEqual(self.mc.get_multi(keys), {})

    def test_append_multi_performance(self):
        N = 70000
        K = "test_append_multi_%d"
        data = b"after\n"
        keys = [K % i for i in range(N)]
        t = time.time()
        self.mc.append_multi(keys, data)
        t = time.time() - t
        threshold = 5
        assert t < threshold, 'should append 7w key in %s secs, ' \
                              'actual val: %f' % (threshold, t)

    def test_get_host(self):
        host = self.mc.get_host_by_key("str")
        self.assertEqual(host, self.server_addr)

    def test_get_list(self):
        self.mc.set("a", 'a')
        self.mc.delete('b')
        v = self.mc.get_list(['a', 'b'])
        self.assertEqual(v, ['a', None])

    @pytest.mark.skipif(np is None or cmemcached is None,
                        reason='cmemcached and numpy are not installed')
    def test_general_get_set_regression(self):
        key = 'anykey'
        key_dup = '%s_dup' % key
        for val in ('x', np.uint32(1), np.int32(2), 0, int(0), False, True):
            self.old_mc.set(key, val)
            val2 = self.mc.get(key)
            assert val2 == val
            self.mc.set(key_dup, val)
            val3 = self.old_mc.get(key_dup)
            assert val3 == val

    @pytest.mark.skipif(cmemcached is None,
                        reason='cmemcached is not installed')
    def test_large_mc_split_regression(self):
        key = 'anykey'
        key_dup = '%s_dup' % key
        for val in ('i' * int(_DOUBAN_CHUNK_SIZE * 1.5), 'small_value'):
            self.old_mc.set(key, val)
            assert self.mc.get(key) == self.old_mc.get(key) == val
            self.mc.set(key, val)
            assert self.mc.get(key) == self.old_mc.get(key) == val

            raw_val1, flags1 = self.old_mc.get_raw(key)
            assert len(raw_val1) <= len(val)  # compressed
            assert self.old_mc.set_raw(key_dup, raw_val1, 0, flags1)
            assert self.old_mc.get_raw(key) == self.old_mc.get_raw(key_dup)

            raw_val2, flags2 = self.mc.get_raw(key)
            assert len(raw_val2) <= len(val)  # compressed
            assert self.mc.set_raw(key_dup, raw_val2, 0, flags2)
            assert self.mc.get_raw(key) == self.mc.get_raw(key_dup)


class CmemcachedRegressionPrefixCase(unittest.TestCase):

    def setUp(self):
        host = "127.0.0.1"
        port = 21211
        self.prefix = '/prefix'
        self.server_addr = '%s:%d' % (host, port)
        self.prefixed_mc = Client([self.server_addr], comp_threshold=1024,
                                  prefix=self.prefix)
        self.mc = Client([self.server_addr], comp_threshold=1024)

    def test_duplicate_prefix_text(self):
        for case in ['%sforever/young', 'forever%s/young', 'forever/young/%s']:
            nasty_key = case % self.prefix
            self.prefixed_mc.set(nasty_key, 1)
            self.assertEqual(self.prefixed_mc.get(nasty_key), 1)
            self.assertEqual(self.prefixed_mc.get_multi([nasty_key]),
                             {nasty_key: 1})

    def test_misc(self):
        raw_mc = self.mc
        prefixed_mc = self.prefixed_mc

        raw_mc.delete('a')
        prefixed_mc.set('a', 1)
        assert(prefixed_mc.get('a') == 1)
        assert(raw_mc.get(self.prefix + 'a') == 1)
        assert(raw_mc.get('a') is None)

        prefixed_mc.add('b', 2)
        assert(prefixed_mc.get('b') == 2)
        assert(raw_mc.get(self.prefix + 'b') == 2)
        assert(raw_mc.get('b') is None)

        prefixed_mc.incr('b')
        assert(prefixed_mc.get('b') == 3)
        assert(raw_mc.get(self.prefix + 'b') == 3)

        raw_mc.decr(self.prefix + 'b')
        assert(prefixed_mc.get('b') == 2)

        prefixed_mc.set_multi({'x': 'a', 'y': 'b'})
        ret = prefixed_mc.get_multi(['x', 'y'])
        assert(ret.get('x') == 'a' and ret.get('y') == 'b')
        assert(prefixed_mc.delete_multi(['a', 'b', 'x', 'y']))

        prefixed_mc.set('?foo', 'bar')
        assert prefixed_mc.get('?foo') == 'bar'
        assert prefixed_mc.get_multi(['?foo']) == {'?foo': 'bar'}
        assert raw_mc.get('?%sfoo' % self.prefix) == 'bar'

        prefixed_mc.set_multi({'?bar': 'foo'})
        assert prefixed_mc.get('?bar') == 'foo'
        assert raw_mc.get('?%sbar' % self.prefix) == 'foo'
        assert raw_mc.get_list(['?%sbar' % self.prefix]) == ['foo']

    def test_ketama(self):
        mc = Client(
            ['localhost', 'myhost:11211', '127.0.0.1:11212', 'myhost:11213'])
        rs = {
            'test:10000': 'localhost:11211',
            'test:20000': '127.0.0.1:11212',
            'test:30000': '127.0.0.1:11212',
            'test:40000': '127.0.0.1:11212',
            'test:50000': '127.0.0.1:11212',
            'test:60000': 'myhost:11213',
            'test:70000': '127.0.0.1:11212',
            'test:80000': '127.0.0.1:11212',
            'test:90000': '127.0.0.1:11212',
        }
        for k in rs:
            self.assertEqual(mc.get_host_by_key(k), rs[k])

    def test_ketama_with_jocky_alias(self):
        mc = Client([
            'localhost localhost',
            'myhost:11211 myhost',
            '127.0.0.1:11212 127.0.0.1:11212',
            'myhost:11213 myhost:11213'
        ])
        rs = {
            'test:10000': 'localhost',
            'test:20000': '127.0.0.1:11212',
            'test:30000': '127.0.0.1:11212',
            'test:40000': '127.0.0.1:11212',
            'test:50000': '127.0.0.1:11212',
            'test:60000': 'myhost:11213',
            'test:70000': '127.0.0.1:11212',
            'test:80000': '127.0.0.1:11212',
            'test:90000': '127.0.0.1:11212',
        }
        for k in rs:
            self.assertEqual(mc.get_host_by_key(k), rs[k])

    def test_ketama_with_alias(self):
        mc = Client([
            '192.168.1.211:11211 tango.mc.douban.com',
            '192.168.1.212:11212 uniform.mc.douban.com',
            '192.168.1.211:11212 victor.mc.douban.com',
            '192.168.1.212:11211 whiskey.mc.douban.com',
        ])
        rs = {
            'test:10000': 'whiskey.mc.douban.com',
            'test:20000': 'victor.mc.douban.com',
            'test:30000': 'victor.mc.douban.com',
            'test:40000': 'victor.mc.douban.com',
            'test:50000': 'victor.mc.douban.com',
            'test:60000': 'uniform.mc.douban.com',
            'test:70000': 'tango.mc.douban.com',
            'test:80000': 'victor.mc.douban.com',
            'test:90000': 'victor.mc.douban.com',
        }
        for k in rs:
            self.assertEqual(mc.get_host_by_key(k), rs[k])

    def test_prefixed_ketama(self):
        mc = Client(
            ['localhost', 'myhost:11211', '127.0.0.1:11212', 'myhost:11213'],
            prefix="/prefix"
        )
        rs = {
            'test:10000': '127.0.0.1:11212',
            'test:20000': 'localhost:11211',
            'test:30000': 'myhost:11213',
            'test:40000': 'myhost:11211',
            'test:50000': 'myhost:11213',
            'test:60000': 'myhost:11213',
            'test:70000': 'localhost:11211',
            'test:80000': 'myhost:11213',
            'test:90000': 'myhost:11213',
        }
        for k in rs:
            self.assertEqual(mc.get_host_by_key(k), rs[k])

    def test_should_raise_exception_if_called_in_different_thread(self):

        def fn():
            self.assertRaises(ThreadUnsafe, self.mc.set, 'key_thread', 1)

        # make connection in main thread
        self.mc.get('key_thread')

        # use it in another thread (should be forbidden)
        t = threading.Thread(target=fn)
        t.start()
        t.join()

    def test_expire(self):
        self.mc.set('dust', 'cs')
        v1 = self.mc.get('dust')
        self.mc.expire('dust')
        v2 = self.mc.get('dust')
        assert v1 == 'cs'
        assert v2 is None


if __name__ == '__main__':
    unittest.main()
