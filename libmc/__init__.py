import os
import functools
from ._client import (
    PyClient, ThreadUnsafe,
    encode_value,
    decode_value,
    PyClientPool, PyClientUnsafe as ClientUnsafe,

    MC_DEFAULT_EXPTIME,
    MC_POLL_TIMEOUT,
    MC_CONNECT_TIMEOUT,
    MC_RETRY_TIMEOUT,
    MC_SET_FAILOVER,
    MC_INITIAL_CLIENTS,
    MC_MAX_CLIENTS,
    MC_MAX_GROWTH,

    MC_HASH_MD5,
    MC_HASH_FNV1_32,
    MC_HASH_FNV1A_32,
    MC_HASH_CRC_32,

    MC_RETURN_SEND_ERR,
    MC_RETURN_RECV_ERR,
    MC_RETURN_CONN_POLL_ERR,
    MC_RETURN_POLL_TIMEOUT_ERR,
    MC_RETURN_POLL_ERR,
    MC_RETURN_MC_SERVER_ERR,
    MC_RETURN_PROGRAMMING_ERR,
    MC_RETURN_INVALID_KEY_ERR,
    MC_RETURN_INCOMPLETE_BUFFER_ERR,
    MC_RETURN_OK,
    __file__ as _libmc_so_file
)

__VERSION__ = "1.4.12"
__version__ = "1.4.12"
__author__ = "mckelvin"
__email__ = "mckelvin@users.noreply.github.com"
__date__ = "Fri Jun  7 06:16:00 2024 +0800"


class Client(PyClient):
    pass

class ClientPool(PyClientPool):
    pass

class ThreadedClient:
    def __init__(self, *args, **kwargs):
        self._client_pool = ClientPool(*args, **kwargs)

    def update_servers(self, servers):
        return self._client_pool.update_servers(servers)

    def config(self, opt, val):
        self._client_pool.config(opt, val)

    def __getattr__(self, key):
        if not hasattr(Client, key):
            raise AttributeError
        result = getattr(Client, key)
        if callable(result):
            @functools.wraps(result)
            def wrapper(*args, **kwargs):
                with self._client_pool.client() as mc:
                    return getattr(mc, key)(*args, **kwargs)
            return wrapper
        return result


DYNAMIC_LIBRARIES = [os.path.abspath(_libmc_so_file)]


__all__ = [
    'Client', 'ThreadUnsafe', '__VERSION__', 'encode_value', 'decode_value',
    'ClientUnsafe', 'ClientPool', 'ThreadedClient',

    'MC_DEFAULT_EXPTIME', 'MC_POLL_TIMEOUT', 'MC_CONNECT_TIMEOUT',
    'MC_RETRY_TIMEOUT', 'MC_SET_FAILOVER', 'MC_INITIAL_CLIENTS',
    'MC_MAX_CLIENTS', 'MC_MAX_GROWTH',

    'MC_HASH_MD5', 'MC_HASH_FNV1_32', 'MC_HASH_FNV1A_32', 'MC_HASH_CRC_32',

    'MC_RETURN_SEND_ERR', 'MC_RETURN_RECV_ERR', 'MC_RETURN_CONN_POLL_ERR',
    'MC_RETURN_POLL_TIMEOUT_ERR', 'MC_RETURN_POLL_ERR',
    'MC_RETURN_MC_SERVER_ERR', 'MC_RETURN_PROGRAMMING_ERR',
    'MC_RETURN_INVALID_KEY_ERR', 'MC_RETURN_INCOMPLETE_BUFFER_ERR',
    'MC_RETURN_OK', 'DYNAMIC_LIBRARIES'
]
