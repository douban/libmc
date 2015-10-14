# distutils: language = c++
# distutils: include_dirs = ["include", "include/hashkit"]
# cython: profile=False

from libc.stdint cimport uint8_t, uint32_t, uint64_t, int64_t
from libcpp cimport bool as bool_t
from libcpp.string cimport string
from libcpp.vector cimport vector
from cpython cimport PyString_FromStringAndSize, PyString_AsStringAndSize, PyString_AsString, Py_INCREF, Py_DECREF, PyInt_AsLong, PyInt_FromLong
from cpython.mem cimport PyMem_Malloc, PyMem_Free

import os
import traceback
import threading
import zlib
import marshal
import cPickle as pickle


cdef extern from "Common.h" namespace "douban::mc":
    ctypedef enum op_code_t:
        SET_OP
        ADD_OP
        REPLACE_OP
        APPEND_OP
        PREPEND_OP
        CAS_OP
        GET_OP
        GETS_OP
        INCR_OP
        DECR_OP
        TOUCH_OP
        DELETE_OP
        STATS_OP
        FLUSHALL_OP
        VERSION_OP
        QUIT_OP


cdef extern from "Export.h":
    ctypedef enum config_options_t:
        CFG_POLL_TIMEOUT
        CFG_CONNECT_TIMEOUT
        CFG_RETRY_TIMEOUT
        CFG_HASH_FUNCTION

    ctypedef enum hash_function_options_t:
        OPT_HASH_MD5
        OPT_HASH_FNV1_32
        OPT_HASH_FNV1A_32
        OPT_HASH_CRC_32

    ctypedef int64_t exptime_t
    ctypedef uint32_t flags_t
    ctypedef uint64_t cas_unique_t

    ctypedef struct retrieval_result_t:
        retrieval_result_t()
        char* key
        uint8_t key_len
        flags_t flags
        char* data_block
        uint32_t bytes
        cas_unique_t cas_unique

    ctypedef struct broadcast_result_t:
        char* host
        char** lines
        size_t* line_lens
        size_t len

    ctypedef enum message_result_type:
        MSG_EXISTS
        MSG_OK
        MSG_STORED
        MSG_NOT_STORED
        MSG_NOT_FOUND
        MSG_DELETED
        MSG_TOUCHED

    ctypedef struct message_result_t:
        message_result_type type_
        char* key
        size_t key_len

    ctypedef enum err_code_t:
        RET_SEND_ERR
        RET_RECV_ERR
        RET_CONN_POLL_ERR
        RET_POLL_TIMEOUT_ERR
        RET_POLL_ERR
        RET_MC_SERVER_ERR
        RET_PROGRAMMING_ERR
        RET_INVALID_KEY_ERR
        RET_INCOMPLETE_BUFFER_ERR
        RET_OK

    ctypedef struct unsigned_result_t:
        char* key
        size_t key_len
        uint64_t value


cdef extern from "Client.h" namespace "douban::mc":
    cdef cppclass Client:
        Client()
        void config(config_options_t opt, int val) nogil
        int init(const char* const * hosts, const uint32_t* ports, size_t n,
                 const char* const * aliases) nogil
        char* getServerAddressByKey(const char* key, size_t keyLen) nogil
        void enableConsistentFailover() nogil
        void disableConsistentFailover() nogil
        err_code_t get(
            const char* const* keys, const size_t* keyLens, size_t nKeys,
            retrieval_result_t*** results, size_t* nResults
        ) nogil
        err_code_t gets(
            const char* const* keys, const size_t* keyLens, size_t nKeys,
            retrieval_result_t*** results, size_t* nResults
        ) nogil
        void destroyRetrievalResult() nogil

        err_code_t set(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t add(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t replace(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t prepend(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t append(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t cas(
            const char* const* keys, const size_t* key_lens,
            const flags_t* flags, const exptime_t exptime,
            const cas_unique_t* cas_uniques, const bool_t noreply,
            const char* const* vals, const size_t* val_lens,
            size_t n_items, message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t _delete(
            const char* const* keys, const size_t* key_lens,
            const bool_t noreply, size_t n_items,
            message_result_t*** results, size_t* nResults
        ) nogil
        err_code_t touch(
            const char* const* keys, const size_t* keyLens,
            const exptime_t exptime, const bool_t noreply, size_t nItems,
            message_result_t*** results, size_t* nResults
        ) nogil
        void destroyMessageResult() nogil

        err_code_t version(broadcast_result_t** results, size_t* nHosts) nogil
        err_code_t quit() nogil
        err_code_t stats(broadcast_result_t** results, size_t* nHosts) nogil
        void destroyBroadcastResult() nogil

        err_code_t incr(
            const char* key, const size_t keyLen, const uint64_t delta,
            const bool_t noreply, unsigned_result_t** results,
            size_t* nResults
        ) nogil
        err_code_t decr(
            const char* key, const size_t keyLen, const uint64_t delta,
            const bool_t noreply, unsigned_result_t** results,
            size_t* nResults
        ) nogil
        void destroyUnsignedResult() nogil
        void _sleep(uint32_t ms) nogil

cdef uint32_t MC_DEFAULT_PORT = 11211
cdef flags_t _FLAG_EMPTY = 0
cdef flags_t _FLAG_PICKLE = 1 << 0
cdef flags_t _FLAG_INTEGER = 1 << 1
cdef flags_t _FLAG_LONG = 1 << 2
cdef flags_t _FLAG_BOOL = 1 << 3
cdef flags_t _FLAG_COMPRESS = 1 << 4
cdef flags_t _FLAG_MARSHAL = 1 << 5
cdef exptime_t DEFAULT_EXPTIME = 0
cdef cas_unique_t DEFAULT_CAS_UNIQUE = 0

cdef flags_t _FLAG_DOUBAN_CHUNKED = 1 << 12
cdef int _DOUBAN_CHUNK_SIZE = 1000000


MC_DEFAULT_EXPTIME = PyInt_FromLong(DEFAULT_EXPTIME)
MC_POLL_TIMEOUT = PyInt_FromLong(CFG_POLL_TIMEOUT)
MC_CONNECT_TIMEOUT = PyInt_FromLong(CFG_CONNECT_TIMEOUT)
MC_RETRY_TIMEOUT = PyInt_FromLong(CFG_RETRY_TIMEOUT)


MC_HASH_MD5 = PyInt_FromLong(OPT_HASH_MD5)
MC_HASH_FNV1_32 = PyInt_FromLong(OPT_HASH_FNV1_32)
MC_HASH_FNV1A_32 = PyInt_FromLong(OPT_HASH_FNV1A_32)
MC_HASH_CRC_32 = PyInt_FromLong(OPT_HASH_CRC_32)


MC_RETURN_SEND_ERR = PyInt_FromLong(RET_SEND_ERR)
MC_RETURN_RECV_ERR = PyInt_FromLong(RET_RECV_ERR)
MC_RETURN_CONN_POLL_ERR = PyInt_FromLong(RET_CONN_POLL_ERR)
MC_RETURN_POLL_TIMEOUT_ERR = PyInt_FromLong(RET_POLL_TIMEOUT_ERR)
MC_RETURN_POLL_ERR = PyInt_FromLong(RET_POLL_ERR)
MC_RETURN_MC_SERVER_ERR = PyInt_FromLong(RET_MC_SERVER_ERR)
MC_RETURN_PROGRAMMING_ERR = PyInt_FromLong(RET_PROGRAMMING_ERR)
MC_RETURN_INVALID_KEY_ERR = PyInt_FromLong(RET_INVALID_KEY_ERR)
MC_RETURN_INCOMPLETE_BUFFER_ERR = PyInt_FromLong(RET_INCOMPLETE_BUFFER_ERR)
MC_RETURN_OK = PyInt_FromLong(RET_OK)


cdef dict ERROR_CODE_TO_STR = {
    MC_RETURN_SEND_ERR: 'send_error',
    MC_RETURN_RECV_ERR: 'recv_error',
    MC_RETURN_CONN_POLL_ERR: 'conn_poll_error',
    MC_RETURN_POLL_TIMEOUT_ERR: 'poll_timeout_error',
    MC_RETURN_POLL_ERR: 'poll_error',
    MC_RETURN_MC_SERVER_ERR: 'server_error',
    MC_RETURN_PROGRAMMING_ERR: 'programming_error',
    MC_RETURN_INVALID_KEY_ERR: 'invalid_key_error',
    MC_RETURN_INCOMPLETE_BUFFER_ERR: 'incomplete_buffer_error',
    MC_RETURN_OK: 'ok'
}


cdef bytes _encode_value(object val, int comp_threshold, flags_t *flags):
    type_ = type(val)
    cdef bytes enc_val = None
    if type_ is bytes:
        flags[0] = _FLAG_EMPTY
        enc_val = val
    elif type_ is bool:
        flags[0] = _FLAG_BOOL
        enc_val = b'1' if val else b'0'
    elif type_ is int:
        flags[0] = _FLAG_INTEGER
        enc_val = bytes(val)
    elif type_ is long:
        flags[0] = _FLAG_LONG
        enc_val = bytes(val)
    elif type_.__module__ == 'numpy':
        enc_val = pickle.dumps(val, -1)
        flags[0] = _FLAG_PICKLE
    else:
        try:
            enc_val = marshal.dumps(val, 2)
            flags[0] = _FLAG_MARSHAL
        except:
            try:
                enc_val = pickle.dumps(val, -1)
                flags[0] = _FLAG_PICKLE
            except:
                pass

    if comp_threshold > 0 and enc_val is not None and len(enc_val) > comp_threshold:
        enc_val = zlib.compress(enc_val)
        flags[0] |= _FLAG_COMPRESS

    return enc_val


def encode_value(object val, int comp_threshold):
    cdef flags_t flags
    cdef bytes buf = _encode_value(val, comp_threshold, &flags)
    return buf, flags


cpdef object decode_value(bytes val, flags_t flags):
    cdef object dec_val = None
    if flags & _FLAG_COMPRESS:
        try:
            dec_val = zlib.decompress(val)
        except zlib.error:
            return
    else:
        dec_val = val

    if flags & _FLAG_BOOL:
        dec_val = bool(int(dec_val))
    elif flags & _FLAG_INTEGER:
        dec_val = int(dec_val)
    elif flags & _FLAG_LONG:
        dec_val = long(dec_val)
    elif flags & _FLAG_MARSHAL:
        try:
            dec_val = marshal.loads(dec_val)
        except:
            dec_val = None
    elif flags & _FLAG_PICKLE:
        try:
            dec_val = pickle.loads(dec_val)
        except:
            dec_val = None
    return dec_val


class ThreadUnsafe(Exception):
    pass


cdef class PyClient:
    cdef readonly list servers
    cdef readonly int comp_threshold
    cdef Client* _imp
    cdef bool_t do_split
    cdef bool_t noreply
    cdef bytes prefix
    cdef int last_error
    cdef object _thread_ident
    cdef object _created_stack

    def __cinit__(self, list servers, bool_t do_split=True, int comp_threshold=0, noreply=False,
                  bytes prefix=None, hash_function_options_t hash_fn=OPT_HASH_MD5, failover=False):
        self.servers = servers
        cdef size_t n = len(servers)
        cdef char** c_hosts = <char**>PyMem_Malloc(n * sizeof(char*))
        cdef uint32_t* c_ports = <uint32_t*>PyMem_Malloc(n * sizeof(uint32_t))
        cdef char** c_aliases = <char**>PyMem_Malloc(n * sizeof(char*))
        servers_ = []
        for srv in servers:
            addr_alias = srv.split(' ')
            addr = addr_alias[0]
            if len(addr_alias) == 1:
                alias = None
            else:
                alias = addr_alias[1]

            host_port = addr.split(':')
            host = host_port[0]
            if len(host_port) == 1:
                port = MC_DEFAULT_PORT
            else:
                port = int(host_port[1])
            servers_.append((host, port, alias))

        Py_INCREF(servers_)
        for i in range(n):
            host, port, alias = servers_[i]
            c_hosts[i] = PyString_AsString(host)
            c_ports[i] = PyInt_AsLong(port)
            if alias is None:
                c_aliases[i] = NULL
            else:
                c_aliases[i] = PyString_AsString(alias)

        cdef int rv = 0
        self._imp = new Client()
        self._imp.config(CFG_HASH_FUNCTION, hash_fn)
        rv = self._imp.init(c_hosts, c_ports, n, c_aliases)
        if failover:
            self._imp.enableConsistentFailover()
        else:
            self._imp.disableConsistentFailover()

        PyMem_Free(c_hosts)
        PyMem_Free(c_ports)
        PyMem_Free(c_aliases)
        self.do_split = do_split
        self.comp_threshold = comp_threshold
        self.noreply = noreply
        self.prefix = prefix
        Py_DECREF(servers_)

        self.last_error = 0
        self._thread_ident = None
        self._created_stack = traceback.extract_stack()

    def __dealloc__(self):
        del self._imp

    def __reduce__(self):
        return (PyClient, (self.servers, self.do_split, self.comp_threshold, self.noreply, self.prefix))

    def config(self, int opt, int val):
        self._imp.config(<config_options_t>opt, val)

    def get_host_by_key(self, basestring key):
        cdef bytes key2 = self.normalize_key(key)
        cdef char* c_key = NULL
        cdef const char* c_addr = NULL
        cdef size_t c_key_len = 0
        Py_INCREF(key2)
        PyString_AsStringAndSize(key2, &c_key, <Py_ssize_t*>&c_key_len)
        with nogil:
            c_addr = self._imp.getServerAddressByKey(c_key, c_key_len)
        cdef bytes c_server_addr = c_addr
        Py_DECREF(key2)
        return c_server_addr


    cdef normalize_key(self, basestring raw_key, encoding='utf-8'):
        if self.prefix:
            if raw_key[0] == '?':
                raw_key = '?' + self.prefix + raw_key[1:]
            else:
                raw_key = self.prefix + raw_key

        type_ = type(raw_key)
        if type_ is unicode:
            return raw_key.encode(encoding)
        elif type_ is bytes:
            return raw_key
        return bytes(raw_key)

    cdef _get_raw(self, op_code_t op, bytes key, flags_t* flags_ptr, cas_unique_t* cas_unique_ptr):
        cdef char* c_key = NULL
        cdef size_t c_key_len = 0
        Py_INCREF(key)
        PyString_AsStringAndSize(key, &c_key, <Py_ssize_t*>&c_key_len)  # XXX: safe cast?
        cdef size_t n = 1, n_results = 0
        cdef retrieval_result_t** results = NULL
        with nogil:
            if op == GET_OP:
                self.last_error = self._imp.get(&c_key, &c_key_len, n, &results, &n_results)
            elif op == GETS_OP:
                self.last_error = self._imp.gets(&c_key, &c_key_len, n, &results, &n_results)
            else:
                pass

        cdef bytes py_value = None
        if n_results == 1:
            py_value = results[0].data_block[:results[0].bytes]
            flags_ptr[0] = results[0].flags
            if op == GETS_OP:
                cas_unique_ptr[0] = results[0].cas_unique
        Py_DECREF(key)
        with nogil:
            self._imp.destroyRetrievalResult()
        return py_value

    def _get_large_raw(self, bytes key, int n_splits, flags_t chuncked_flags):

        cdef size_t len_key = len(key)
        if n_splits > 10 or len_key > 200:
            return (None, 0)

        cdef list keys = ['~%d%s/%d' % (len_key, key, i) for i in range(n_splits)]

        cdef dict dct = self._get_multi_raw(n_splits, keys)
        if len(dct) != n_splits:
            return (None, 0)
        return (b''.join(dct[key][0] for key in keys), chuncked_flags & ~_FLAG_DOUBAN_CHUNKED)

    def get_raw(self, basestring key):
        self._record_thread_ident()
        cdef bytes key2 = self.normalize_key(key)

        cdef flags_t flags = 0
        cdef cas_unique_t cas_unique = 0
        cdef bytes py_value = self._get_raw(GET_OP, key2, &flags, &cas_unique)

        if py_value is not None and self.do_split and (flags & _FLAG_DOUBAN_CHUNKED):
            n_splits = int(py_value.strip('\0'))
            py_value, flags = self._get_large_raw(key2, n_splits, flags)

        return py_value, flags

    def get(self, basestring key):
        py_value, flags = self.get_raw(key)

        if py_value is None:
            return

        return decode_value(py_value, flags)

    def gets(self, basestring key):
        self._record_thread_ident()
        cdef bytes key2 = self.normalize_key(key)

        cdef flags_t flags = 0
        cdef cas_unique_t cas_unique = 0
        cdef bytes py_value = self._get_raw(GETS_OP, key2, &flags, &cas_unique)
        cdef int n_splits = 0

        if py_value is not None and self.do_split and (flags & _FLAG_DOUBAN_CHUNKED):
            n_splits = int(py_value.strip('\0'))
            py_value, flags = self._get_large_raw(key2, n_splits, flags)

        if py_value is None:
            return

        return decode_value(py_value, flags), cas_unique

    def _get_multi_raw(self, size_t n, list keys):
        cdef size_t n_res = 0
        cdef char** c_keys = <char**>PyMem_Malloc(n * sizeof(char*))
        cdef size_t* c_key_lens = <size_t*>PyMem_Malloc(n * sizeof(size_t))
        Py_INCREF(keys)
        for i in range(n):
            PyString_AsStringAndSize(keys[i], &c_keys[i], <Py_ssize_t*>&c_key_lens[i])  # XXX: safe cast?

        cdef retrieval_result_t** results = NULL
        cdef retrieval_result_t *r = NULL
        with nogil:
            self.last_error = self._imp.get(c_keys, c_key_lens, n, &results, &n_res)

        cdef dict rv = {}
        cdef bytes py_key
        cdef bytes py_value
        for i in range(n_res):
            r = results[i]
            py_key = r.key[:r.key_len]
            py_value = r.data_block[:r.bytes]
            flags = r.flags
            rv[py_key] = (py_value, flags)
        PyMem_Free(c_keys)
        PyMem_Free(c_key_lens)
        Py_DECREF(keys)
        with nogil:
            self._imp.destroyRetrievalResult()
        return rv

    def get_multi(self, keys):
        self._record_thread_ident()
        keys = [self.normalize_key(key) for key in keys]
        cdef size_t n_keys = len(keys)
        cdef dict multi_raw = self._get_multi_raw(n_keys, keys)
        cdef dict dct = dict()
        cdef bytes out_key = None
        cdef int n_splits = 0
        for i in range(n_keys):
            if keys[i] not in multi_raw:
                continue

            raw_bytes, flags = multi_raw[keys[i]]
            out_key = keys[i].replace(self.prefix, '', 1) if self.prefix else keys[i]

            if raw_bytes is not None and self.do_split and (flags & _FLAG_DOUBAN_CHUNKED):
                n_splits = int(raw_bytes.strip('\0'))
                raw_bytes, flags  = self._get_large_raw(keys[i], n_splits, flags)
            if raw_bytes is None:
                continue
            dct[out_key] = decode_value(raw_bytes, flags)

        return dct

    def get_list(self, keys):
        self._record_thread_ident()
        dct = self.get_multi(keys)
        return [dct.get(key) for key in keys]

    cdef _store_raw(self, op_code_t op, bytes key, flags_t flags, exptime_t exptime, bytes val, cas_unique_t cas_unique):
        if val is None:  # val is not pickable
            return None
        cdef char* c_key = NULL
        cdef size_t c_key_len = 0
        cdef char* c_val = NULL
        cdef size_t c_val_len = 0, n_res = 0
        cdef size_t n = 1

        Py_INCREF(key)
        Py_INCREF(val)

        PyString_AsStringAndSize(key, &c_key, <Py_ssize_t*>&c_key_len)
        PyString_AsStringAndSize(val, &c_val, <Py_ssize_t*>&c_val_len)

        cdef message_result_t** results = NULL

        with nogil:
            if op == SET_OP:
                self.last_error = self._imp.set(&c_key, &c_key_len, &flags, exptime, NULL, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            elif op == ADD_OP:
                self.last_error = self._imp.add(&c_key, &c_key_len, &flags, exptime, NULL, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            elif op == REPLACE_OP:
                self.last_error = self._imp.replace(&c_key, &c_key_len, &flags, exptime, NULL, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            elif op == PREPEND_OP:
                self.last_error = self._imp.prepend(&c_key, &c_key_len, &flags, exptime, NULL, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            elif op == APPEND_OP:
                self.last_error = self._imp.append(&c_key, &c_key_len, &flags, exptime, NULL, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            elif op == CAS_OP:
                self.last_error = self._imp.cas(&c_key, &c_key_len, &flags, exptime, &cas_unique, self.noreply, &c_val, &c_val_len, n, &results, &n_res)
            else:
                pass

        rv = self.last_error == 0 and (self.noreply or (n_res == 1 and results[0][0].type_ == MSG_STORED))

        with nogil:
            self._imp.destroyMessageResult()
        Py_DECREF(key)
        Py_DECREF(val)
        return rv

    cdef _set(self, bytes key, bytes val, flags_t flags, exptime_t exptime):
        if val is None:  # val is not pickable
            return
        cdef int c_val_len = len(val)
        if not self.do_split or c_val_len <= _DOUBAN_CHUNK_SIZE:
            return self._store_raw(SET_OP, key, flags, exptime, val, DEFAULT_CAS_UNIQUE)

        len_key = len(key)
        #  split large value
        if c_val_len > 10 * _DOUBAN_CHUNK_SIZE or len_key > 200:
            return False

        a, b = divmod(c_val_len, _DOUBAN_CHUNK_SIZE)
        cdef int n_splits = a + (0 if b == 0 else 1)
        keys = ['~%d%s/%d' % (len_key, key, i) for i in range(n_splits)]
        vals = [val[i:i+_DOUBAN_CHUNK_SIZE] for i in range(0, c_val_len, _DOUBAN_CHUNK_SIZE)]
        cdef flags_t* splits_flags = <flags_t*>PyMem_Malloc(n_splits * sizeof(flags_t))
        for i in range(n_splits):
            splits_flags[i] = flags
        rv = self._store_multi_raw(SET_OP, n_splits, keys, vals, splits_flags, exptime, return_failure=False)
        PyMem_Free(splits_flags)
        if rv is False:
            return False
        return self._store_raw(SET_OP, key, flags | _FLAG_DOUBAN_CHUNKED, exptime, bytes(n_splits), DEFAULT_CAS_UNIQUE)

    def set_raw(self, basestring key, object val, exptime_t time, flags_t flags):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        return self._set(key2, val, flags, time)

    def set(self, basestring key, object val, exptime_t time=DEFAULT_EXPTIME, bool_t compress=True):
        cdef flags_t flags
        comp_threshold = self.comp_threshold if compress else 0
        cdef bytes enc_val = _encode_value(val, comp_threshold, &flags)
        if enc_val is not None:
            return self.set_raw(key, enc_val, time, flags)
        else:
            return None

    def add(self, basestring key, object val, exptime_t time=DEFAULT_EXPTIME):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        cdef flags_t flags
        cdef bytes enc_val = _encode_value(val, self.comp_threshold, &flags)
        return self._store_raw(ADD_OP, key2, flags, time, enc_val, DEFAULT_CAS_UNIQUE)

    def replace(self, basestring key, object val, exptime_t time=DEFAULT_EXPTIME):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        cdef flags_t flags
        cdef bytes enc_val = _encode_value(val, self.comp_threshold, &flags)
        return self._store_raw(REPLACE_OP, key2, flags, time, enc_val, DEFAULT_CAS_UNIQUE)

    def prepend(self, basestring key, object val, compress=False):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        cdef flags_t flags
        comp_threshold = self.comp_threshold if compress else 0
        cdef bytes enc_val = _encode_value(val, comp_threshold, &flags)
        return self._store_raw(PREPEND_OP, key2, flags, DEFAULT_EXPTIME, enc_val, DEFAULT_CAS_UNIQUE)

    def append(self, basestring key, object val, compress=False):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        cdef flags_t flags
        comp_threshold = self.comp_threshold if compress else 0
        cdef bytes enc_val = _encode_value(val, comp_threshold, &flags)
        return self._store_raw(APPEND_OP, key2, flags, DEFAULT_EXPTIME, enc_val, DEFAULT_CAS_UNIQUE)

    cdef _prepend_or_append_multi(self, op_code_t op, list keys, object val, exptime_t exptime, bool_t compress):
        cdef size_t n = len(keys)
        cdef flags_t* c_flags = <flags_t*>PyMem_Malloc(n * sizeof(flags_t))
        comp_threshold = self.comp_threshold if compress else 0
        cdef bytes enc_val = _encode_value(val, comp_threshold, &(c_flags[0]))

        if enc_val is None:  # val is(are) not pickable
            return None

        # cannot append large val
        if self.do_split and len(enc_val) > _DOUBAN_CHUNK_SIZE:
            return None
        cdef list vals = [enc_val for i in range(n)]
        for i in range(1, n):
            c_flags[i] = c_flags[0]

        rv = self._store_multi_raw(op, n, keys, vals, c_flags, exptime, return_failure=False)
        PyMem_Free(c_flags)
        return rv

    def prepend_multi(self, keys, object val, exptime_t exptime=DEFAULT_EXPTIME, bool_t compress=False):
        self._record_thread_ident()
        self._check_thread_ident()
        return self._prepend_or_append_multi(PREPEND_OP, keys, val, exptime, compress)

    def append_multi(self, keys, object val, exptime_t exptime=DEFAULT_EXPTIME, bool_t compress=False):
        self._record_thread_ident()
        self._check_thread_ident()
        return self._prepend_or_append_multi(APPEND_OP, keys, val, exptime, compress)

    def cas(self, basestring key, object val, exptime_t exptime, cas_unique_t cas_unique):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef bytes key2 = self.normalize_key(key)
        cdef flags_t flags
        cdef bytes enc_val = _encode_value(val, self.comp_threshold, &flags)
        return self._store_raw(CAS_OP, key2, flags, exptime, enc_val, cas_unique)

    cdef _store_multi_raw(self, op_code_t op, size_t n, list keys, list vals, flags_t* c_flags, exptime_t c_exptime, bool_t return_failure):
        Py_INCREF(keys)
        Py_INCREF(vals)
        cdef size_t n_rst = 0
        cdef char** c_keys = <char**>PyMem_Malloc(n * sizeof(char*))
        cdef size_t* c_key_lens = <size_t*>PyMem_Malloc(n * sizeof(size_t))
        cdef char** c_vals = <char**>PyMem_Malloc(n * sizeof(char*))
        cdef size_t* c_val_lens = <size_t*>PyMem_Malloc(n * sizeof(size_t))

        for i in range(n):
            PyString_AsStringAndSize(keys[i], &c_keys[i], <Py_ssize_t*>&c_key_lens[i])  # XXX: safe cast?
            PyString_AsStringAndSize(vals[i], &c_vals[i], <Py_ssize_t*>&c_val_lens[i])  # XXX: safe cast?

        cdef message_result_t** results = NULL
        with nogil:
            if op == SET_OP:
                self.last_error = self._imp.set(c_keys, c_key_lens, <const flags_t*>c_flags, c_exptime, NULL,
                                   self.noreply, c_vals, c_val_lens, n, &results, &n_rst)
            elif op == PREPEND_OP:
                self.last_error = self._imp.prepend(c_keys, c_key_lens, <const flags_t*>c_flags, c_exptime, NULL,
                                   self.noreply, c_vals, c_val_lens, n, &results, &n_rst)
            elif op == APPEND_OP:
                self.last_error = self._imp.append(c_keys, c_key_lens, <const flags_t*>c_flags, c_exptime, NULL,
                                   self.noreply, c_vals, c_val_lens, n, &results, &n_rst)
            else:
                pass
        is_succeed = self.last_error == 0 and (self.noreply or n_rst == n)
        cdef list failed_keys = []
        if not is_succeed and return_failure:
            succeed_keys = [results[i][0].key[:results[i][0].key_len] for i in range(n_rst) if results[i][0].type_ == MSG_STORED]
            failed_keys = list(set(keys) - set(succeed_keys))

        with nogil:
            self._imp.destroyMessageResult()

        PyMem_Free(c_keys)
        PyMem_Free(c_key_lens)
        PyMem_Free(c_vals)
        PyMem_Free(c_val_lens)

        Py_DECREF(keys)
        Py_DECREF(vals)
        return (is_succeed, failed_keys) if return_failure else is_succeed

    def set_multi(self, dict dct, time=DEFAULT_EXPTIME, compress=True, return_failure=False):
        self._record_thread_ident()
        self._check_thread_ident()
        cdef size_t n = len(dct)
        cdef flags_t* c_flags = <flags_t*>PyMem_Malloc(n * sizeof(flags_t))
        cdef list keys = [self.normalize_key(key) for key in dct.iterkeys()]
        comp_threshold = self.comp_threshold if compress else 0
        cdef list vals = [_encode_value(val, comp_threshold, &(c_flags[i]))
                          for i, val in enumerate(dct.itervalues())]

        cdef list failed_keys = []
        if None in vals:  # 1 or more val(s) in vals is(are) not pickable
            PyMem_Free(c_flags)
            failed_keys = keys
            return (False, failed_keys) if return_failure else False

        if not self.do_split or all(len(val) <= _DOUBAN_CHUNK_SIZE for val in vals):
            rv = self._store_multi_raw(SET_OP, n, keys, vals, c_flags, time, return_failure=return_failure)
            PyMem_Free(c_flags)
            return rv

        cdef flags_t* c_flags_small = <flags_t*>PyMem_Malloc(n * sizeof(flags_t))
        cdef list keys_small = []
        cdef list vals_small = []

        cdef size_t j_small = 0
        is_succeed = True
        for i in range(n):
            if len(vals[i]) <= _DOUBAN_CHUNK_SIZE:
                keys_small.append(keys[i])
                vals_small.append(vals[i])
                c_flags_small[j_small] = c_flags[i]
                j_small += 1
            else:
                is_single_succeed = self._set(keys[i], vals[i], c_flags[i], time)
                if not is_single_succeed and return_failure:
                    failed_keys.append(keys[i])
                is_succeed = is_succeed and is_single_succeed

        if j_small > 0U:
            rv2 = self._store_multi_raw(SET_OP, j_small, keys_small, vals_small, c_flags_small, time, return_failure=return_failure)
            if return_failure:
                is_succeed = is_succeed and rv2[0]
                failed_keys.extend(rv2[1])
            else:
                is_succeed = is_succeed and rv2

        PyMem_Free(c_flags)
        PyMem_Free(c_flags_small)
        return (is_succeed, failed_keys) if return_failure else is_succeed


    cdef _delete_raw(self, basestring key):
        cdef char* c_key = NULL
        cdef size_t c_key_len = 0
        cdef size_t n = 1, n_res = 0

        Py_INCREF(key)
        PyString_AsStringAndSize(key, &c_key, <Py_ssize_t*>&c_key_len)

        cdef message_result_t** results = NULL
        with nogil:
            self.last_error = self._imp._delete(&c_key, &c_key_len, self.noreply, n, &results, &n_res)

        rv = self.last_error == 0 and (self.noreply or (n_res == 1 and (results[0][0].type_ == MSG_DELETED or results[0][0].type_ == MSG_NOT_FOUND)))

        with nogil:
            self._imp.destroyMessageResult()
        Py_DECREF(key)
        return rv

    def delete(self, basestring key):
        self._record_thread_ident()
        self._check_thread_ident()
        key = self.normalize_key(key)
        return self._delete_raw(key)

    cdef _delete_multi_raw(self, keys, return_failure):
        cdef size_t n = len(keys), n_res = 0
        cdef char** c_keys = <char**>PyMem_Malloc(n * sizeof(char*))
        cdef size_t* c_key_lens = <size_t*>PyMem_Malloc(n * sizeof(size_t))
        Py_INCREF(keys)
        for i in range(n):
            PyString_AsStringAndSize(keys[i], &c_keys[i], <Py_ssize_t*>&c_key_lens[i])  # XXX: safe cast?

        cdef message_result_t** results = NULL
        cdef message_result_t *r = NULL

        with nogil:
            self.last_error = self._imp._delete(c_keys, c_key_lens, self.noreply, n, &results, &n_res)

        is_succeed = self.last_error == 0 and (self.noreply or n_res == n)
        cdef list failed_keys = []

        if not is_succeed and return_failure:
            succeed_keys = [results[i][0].key[:results[i][0].key_len]
                            for i in range(n_res)
                            if results[i][0].type_ == MSG_DELETED or results[i][0].type_ == MSG_NOT_FOUND]
            failed_keys = list(set(keys) - set(succeed_keys))

        with nogil:
            self._imp.destroyMessageResult()
        PyMem_Free(c_key_lens)
        PyMem_Free(c_keys)
        Py_DECREF(keys)
        return (is_succeed, failed_keys) if return_failure else is_succeed

    def delete_multi(self, keys, return_failure=False):
        self._record_thread_ident()
        self._check_thread_ident()
        keys = [self.normalize_key(key) for key in keys]
        return self._delete_multi_raw(keys, return_failure)

    cdef _touch_raw(self, basestring key, exptime_t exptime):
        cdef char* c_key = NULL
        cdef size_t c_key_len = 0
        cdef size_t n = 1, n_res = 0

        Py_INCREF(key)
        PyString_AsStringAndSize(key, &c_key, <Py_ssize_t*>&c_key_len)

        cdef message_result_t** results = NULL

        with nogil:
            self.last_error = self._imp.touch(&c_key, &c_key_len, exptime, self.noreply, n, &results, &n_res)

        rv = self.last_error == 0 and (self.noreply or (n_res == 1 and results[0][0].type_ == MSG_TOUCHED))
        with nogil:
            self._imp.destroyMessageResult()
        Py_DECREF(key)
        return rv

    def touch(self, basestring key, exptime_t exptime):
        self._record_thread_ident()
        self._check_thread_ident()
        key = self.normalize_key(key)
        return self._touch_raw(key, exptime)

    def version(self):
        self._record_thread_ident()
        cdef broadcast_result_t* rst = NULL
        cdef size_t n = 0
        with nogil:
            self.last_error = self._imp.version(&rst, &n)

        rv = {}
        for i in range(n):
            if rst[i].lines == NULL or rst[i].line_lens == NULL:
                continue
            rv[rst[i].host] = rst[i].lines[0][:rst[i].line_lens[0]]

        with nogil:
            self._imp.destroyBroadcastResult()
        return rv

    def quit(self):
        self._record_thread_ident()
        with nogil:
            self.last_error = self._imp.quit()
            self._imp.destroyBroadcastResult()
        if self.last_error in {RET_CONN_POLL_ERR, RET_RECV_ERR}:
            return True
        return False

    def stats(self):
        self._record_thread_ident()
        cdef broadcast_result_t* rst = NULL
        cdef broadcast_result_t* r = NULL
        cdef size_t n = 0
        rv = {}
        with nogil:
            self.last_error = self._imp.stats(&rst, &n)

        for i in range(n):
            r = &rst[i]
            rv[r.host] = dict()
            for j in range(r.len):
                if r.lines == NULL or r.line_lens == NULL:
                    continue
                line = r.lines[j][:r.line_lens[j]]
                k, v = line.split(' ', 1)
                for dt in (int, float):
                    try:
                        v = dt(v)
                        break
                    except:
                        pass
                rv[r.host][k] = v
        with nogil:
            self._imp.destroyBroadcastResult()
        return rv

    def _incr_decr_raw(self, op_code_t op, bytes key, uint64_t delta):

        cdef char* c_key = NULL
        cdef size_t c_key_len = 0

        Py_INCREF(key)
        PyString_AsStringAndSize(key, &c_key, <Py_ssize_t*>&c_key_len)

        cdef unsigned_result_t* result = NULL
        cdef size_t n_res = 0
        with nogil:
            if op == INCR_OP:
                self.last_error = self._imp.incr(c_key, c_key_len, delta, self.noreply, &result, &n_res)
            elif op == DECR_OP:
                self.last_error = self._imp.decr(c_key, c_key_len, delta, self.noreply, &result, &n_res)
            else:
                pass

        rv = None
        if n_res == 1 and result != NULL:
            rv = result.value
        with nogil:
            self._imp.destroyUnsignedResult()
        Py_DECREF(key)
        return rv

    def incr(self, basestring key, delta=1):
        self._record_thread_ident()
        self._check_thread_ident()
        key = self.normalize_key(key)
        return self._incr_decr_raw(INCR_OP, key, delta)

    def decr(self, basestring key, delta=1):
        self._record_thread_ident()
        self._check_thread_ident()
        key = self.normalize_key(key)
        return self._incr_decr_raw(DECR_OP, key, delta)

    def _sleep(self, uint32_t seconds, release_gil=False):
        if release_gil:
            with nogil:
                self._imp._sleep(seconds)
        else:
            self._imp._sleep(seconds)

    def expire(self, basestring key):
        self._record_thread_ident()
        return self.touch(key, -1)

    def reset(self):
        self.clear_thread_ident()

    def clear_thread_ident(self):
        self._thread_ident = None
        self._thread_ident_stack = None

    def _record_thread_ident(self):
        if self._thread_ident is None:
            self._thread_ident = self._get_current_thread_ident()

    def _check_thread_ident(self):
        if self._get_current_thread_ident() != self._thread_ident:
            raise ThreadUnsafe("mc client created in %s\n%s, called in %s" %
                               (self._thread_ident,
                                self._created_stack,
                                self._get_current_thread_ident()))

    def _get_current_thread_ident(self):
        return (os.getpid(), threading.current_thread().name)

    def get_last_error(self):
        return self.last_error

    def get_last_strerror(self):
        return ERROR_CODE_TO_STR.get(self.last_error, '')
