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
__version__ = "c5566db"
__author__ = "mckelvin"
__email__ = "kelvin0576@gmail.com"
__date__ = "Sat Apr 4 13:07:39 2015 +0800"


class Client(PyClient):
    pass


__all__ = [
    'Client', 'ThreadUnsafe', '__VERSION__', 'encode_value',

    'MC_DEFAULT_EXPTIME', 'MC_POLL_TIMEOUT', 'MC_CONNECT_TIMEOUT',
    'MC_RETRY_TIMEOUT',

    'MC_HASH_MD5', 'MC_HASH_FNV1_32', 'MC_HASH_FNV1A_32', 'MC_HASH_CRC_32'
]
