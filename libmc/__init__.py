from ._client import (
    PyClient, ThreadUnsafe,
    encode_value,

    MC_DEFAULT_EXPTIME,
    MC_POLL_TIMEOUT,
    MC_CONNECT_TIMEOUT,
    MC_RETRY_TIMEOUT,

    MC_HASH_MD5,
    MC_HASH_FNV1_32,
    MC_HASH_FNV1A_32,
    MC_HASH_CRC_32,
)

__VERSION__ = '0.5.0'
__version__ = "v0.5.0-3-gdce6e6f"
__author__ = "PAN, Myautsai"
__email__ = "mckelvin@users.noreply.github.com"
__date__ = "Sun Apr 5 11:48:29 2015 +0800"


class Client(PyClient):
    pass


__all__ = [
    'Client', 'ThreadUnsafe', '__VERSION__', 'encode_value',

    'MC_DEFAULT_EXPTIME', 'MC_POLL_TIMEOUT', 'MC_CONNECT_TIMEOUT',
    'MC_RETRY_TIMEOUT',

    'MC_HASH_MD5', 'MC_HASH_FNV1_32', 'MC_HASH_FNV1A_32', 'MC_HASH_CRC_32'
]
