import os
import sys
import binascii
import hashlib
import numpy as np

from builtins import str as unicode

FNV_32_INIT = 0x811c9dc5
FNV_32_PRIME = 0x01000193

def compute_crc_32(key):
    return np.uint32(binascii.crc32(key))


def compute_fnv1_32(key):
    hval = FNV_32_INIT
    fnv_32_prime = FNV_32_PRIME
    uint32_max = 2 ** 32
    for s in key:
        hval = (hval * fnv_32_prime) % uint32_max
        hval = hval ^ (ord(s) if isinstance(s, str) else s)
    return np.uint32(hval)


def compute_fnv1a_32(key):
    hval = FNV_32_INIT
    fnv_32_prime = FNV_32_PRIME
    uint32_max = 2 ** 32
    for s in key:
        hval = hval ^ (ord(s) if isinstance(s, str) else s)
        hval = (hval * fnv_32_prime) % uint32_max
    return np.uint32(hval)


def compute_md5(key):
    md5 = hashlib.md5()
    md5.update(key)
    digest = md5.digest()

    def convert(d):
        if isinstance(d, int):
            return np.uint32(d & 0xFF)
        else:
            return np.uint32(ord(d) & 0xFF)

    return ((convert(digest[3]) << 24) |
           (convert(digest[2]) << 16) |
           (convert(digest[1]) << 8) |
           (convert(digest[0])))


U32_HASH_FN_DICT = {
    'crc_32': (compute_crc_32, []),
    'fnv1_32': (compute_fnv1_32, []),
    'fnv1a_32': (compute_fnv1a_32, []),
    'md5': (compute_md5, []),
}

def main(argv):
    if not len(argv) == 2:
        print("usage: python %s in.txt" % argv[0])
        return 1

    input_path = os.path.abspath(sys.argv[1])

    with open(input_path, 'rb') as fhandler:
        for line in fhandler:
            key = line.strip()
            for hash_name, (hash_fn, hash_rst) in U32_HASH_FN_DICT.items():
                hash_rst.append(hash_fn(key))

    for hash_name, (hash_fn, hash_rst) in U32_HASH_FN_DICT.items():
        prefix, dot_ext = os.path.splitext(input_path)
        out_path = '%s_%s%s' % (prefix, hash_name, dot_ext)
        with open(out_path, 'w') as fhandler:
            fhandler.writelines(("%s\r\n" % h for h in hash_rst))
        print(out_path)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
