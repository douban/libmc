import os
from ._client import (
    PyClient, ThreadUnsafe,
    encode_value,
    decode_value,

    MC_DEFAULT_EXPTIME,
    MC_POLL_TIMEOUT,
    MC_CONNECT_TIMEOUT,
    MC_RETRY_TIMEOUT,

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

__VERSION__ = "1.4.2"
__version__ = "v1.4.2"
__author__ = "mckelvin"
__email__ = "mckelvin@users.noreply.github.com"
__date__ = "Thu May 20 18:44:42 2021 +0800"


class Client(PyClient):
    pass


DYNAMIC_LIBRARIES = [os.path.abspath(_libmc_so_file)]


__all__ = [
    'Client', 'ThreadUnsafe', '__VERSION__', 'encode_value', 'decode_value',

    'MC_DEFAULT_EXPTIME', 'MC_POLL_TIMEOUT', 'MC_CONNECT_TIMEOUT',
    'MC_RETRY_TIMEOUT',

    'MC_HASH_MD5', 'MC_HASH_FNV1_32', 'MC_HASH_FNV1A_32', 'MC_HASH_CRC_32',

    'MC_RETURN_SEND_ERR', 'MC_RETURN_RECV_ERR', 'MC_RETURN_CONN_POLL_ERR',
    'MC_RETURN_POLL_TIMEOUT_ERR', 'MC_RETURN_POLL_ERR',
    'MC_RETURN_MC_SERVER_ERR', 'MC_RETURN_PROGRAMMING_ERR',
    'MC_RETURN_INVALID_KEY_ERR', 'MC_RETURN_INCOMPLETE_BUFFER_ERR',
    'MC_RETURN_OK', 'DYNAMIC_LIBRARIES'
]
