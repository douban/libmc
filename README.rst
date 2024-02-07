libmc
=====

|build_go| |build_py|
|status| |pypiv| |pyversions| |wheel| |license|

libmc is a memcached client library for Python without any other
dependencies at runtime. It's mainly written in C++ and Cython and
can be considered a drop in replacement for libmemcached and
`python-libmemcached <https://github.com/douban/python-libmemcached>`__.

libmc is developed and maintained by Douban Inc. Currently, it is
working in a production environment, powering all web traffic on
`douban.com <https://www.semrush.com/website/douban.com/overview/>`. Realtime
`benchmark results <https://travis-ci.org/douban/libmc/builds/57124335#L1611>`__
are available on travis.

Build and Installation
----------------------

For users:

::

    pip install libmc

Usage:

.. code:: python

    import libmc

    mc = libmc.Client(['localhost:11211', 'localhost:11212'])
    mc.set('foo', 'bar')
    assert mc.get('foo') == 'bar'

Under the hood
--------------

Under the hood, libmc consists of 2 parts: an internal, fully-functional
memcached client implementation in C++ and a Cython wrapper around that
implementation. Dynamic memory allocation and memory-copy are slow, so
we've tried our best to avoid them. libmc also supports the ``set_multi``
command, which is not natively supported by the `memcached
protocol <https://github.com/memcached/memcached/blob/master/doc/protocol.txt>`__.
Some techniques have been applied to make ``set_multi`` command extremely fast
in libmc (compared to similiar libraries).

Configuration
-------------

.. code:: python

    import libmc
    from libmc import (
        MC_HASH_MD5, MC_POLL_TIMEOUT, MC_CONNECT_TIMEOUT, MC_RETRY_TIMEOUT
    )

    mc = libmc.Client(
        [
        'localhost:11211',
        'localhost:11212',
        'remote_host',
        'remote_host mc.mike',
        'remote_host:11213 mc.oscar'
        ],
        do_split=True,
        comp_threshold=0,
        noreply=False,
        prefix=None,
        hash_fn=MC_HASH_MD5,
        failover=False
    )

    mc.config(MC_POLL_TIMEOUT, 100)  # 100 ms
    mc.config(MC_CONNECT_TIMEOUT, 300)  # 300 ms
    mc.config(MC_RETRY_TIMEOUT, 5)  # 5 s


-  ``servers``: a list of memcached server addresses. Each address
   should be formated as ``hostname[:port] [alias]``, where ``port`` and
   ``alias`` are optional. If ``port`` is not given, the default port ``11211``
   will be used. If given, ``alias`` will be used to compute the server hash,
   which would otherwise be computed based on ``host`` and ``port``
   (i.e. whichever portion is given).
-  ``do_split``: splits large values (up to 10MB) into chunks (&lt;1MB). The
   memcached server implementation will not store items larger than 1MB,
   however in some environments it is beneficial to shard up to 10MB of data.
   Attempts to store more than that are ignored. Default: ``True``.
-  ``comp_threshold``: compresses large values using zlib. If
   ``buffer length > comp_threshold > 0`` (in bytes), the buffer will be
   compressed. If ``comp_threshold == 0``, the string buffer will never be
   compressed. Default: ``0``
-  ``noreply``: controls memcached's
   [``noreply`` feature](https://github.com/memcached/memcached/wiki/CommonFeatures#noreplyquiet).
   default: ``False``
-  ``prefix``: The key prefix. default: ``''``
-  ``hash_fn``: hashing function for keys. possible values:

   -  ``MC_HASH_MD5``
   -  ``MC_HASH_FNV1_32``
   -  ``MC_HASH_FNV1A_32``
   -  ``MC_HASH_CRC_32``

   default: ``MC_HASH_MD5``

   **NOTE:** fnv1\_32, fnv1a\_32, crc\_32 implementations in libmc are
   per each spec, but they're not compatible with corresponding
   implementions in libmemcached.

-  ``failover``: Whether to failover to next server when current server
   is not available. Default: ``False``

-  ``MC_POLL_TIMEOUT`` Timeout parameter used during set/get procedure.
   Default: ``300`` ms
-  ``MC_CONNECT_TIMEOUT`` Timeout parameter used when connecting to
   memcached server in the initial phase. Default: ``100`` ms
-  ``MC_RETRY_TIMEOUT`` When a server is not available due to server-end
   error, libmc will try to establish the broken connection in every
   ``MC_RETRY_TIMEOUT`` s until the connection is back to live. Default:
   ``5`` s

**NOTE:** The hashing algorithm for host mapping on continuum is always
md5.

Contributing to libmc
---------------------

Feel free to send a **Pull Request**. For feature requests or any
questions, please open an **Issue**.

For **SECURITY DISCLOSURE**, please disclose the information responsibly
by sending an email to security@douban.com directly instead of creating
a GitHub issue.

FAQ
---

Does libmc support PHP?
^^^^^^^^^^^^^^^^^^^^^^^

No, but, if you like, you can write a wrapper for PHP based on the C++
implementation.

Is Memcached binary protocol supported ?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

No. Only Memcached ASCII protocol is supported currently.

Why reinventing the wheel?
^^^^^^^^^^^^^^^^^^^^^^^^^^

Before libmc, we were using
`python-libmemcached <https://github.com/douban/python-libmemcached>`__,
which is a python extention for
`libmemcached <http://libmemcached.org/libMemcached.html>`__.
libmemcached is quite weird and buggy. After nearly one decade, there're
still some unsolved bugs.

Is libmc thread-safe ?
^^^^^^^^^^^^^^^^^^^^^^

Yes. `libmc.ThreadedClient` is a thread-safe client implementation. To hold
access for more than one request, `libmc.ClientPool` can be used with Python
`with` statements. `libmc.Client`, however, is a single-threaded memcached
client. If you initialize a standard client in one thread but reuse that in
another thread, a Python ``ThreadUnsafe`` Exception will be raised.

Is libmc compatible with gevent?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Yes, with the help of `greenify <https://github.com/douban/greenify>`__,
libmc is friendly to gevent. Read ``tests/shabby/gevent_issue.py`` for
details. `libmc.ThreadedClient` and `libmc.ClientPool` are not currently
compatible.

**Notice:**

`gevent.monkey.patch_all()` will override
`threading.current_thread().ident` to Greenlet's ID,
this will cause libmc to throw a ThreadUnSafe error
or run into dead lock, you should only patch the things
that you need, e.g.

.. code:: python

    from gevent import monkey
    monkey.patch_socket()

Acknowledgments
---------------

-  Thanks to `@fahrenheit2539 <https://github.com/fahrenheit2539>`__ and
   the llvm project for the standalone.
   `SmallVector <http://fahrenheit2539.blogspot.com/2012/06/introduction-in-depths-look-at.html>`__
   implementation.
-  Thanks to `@miloyip <https://github.com/miloyip>`__ for the high
   performance `i64toa <https://github.com/miloyip/itoa-benchmark>`__
   implementation.
-  Thanks to `Ivan Novikov <https://twitter.com/d0znpp>`__ for the
   research in `THE NEW PAGE OF INJECTIONS BOOK: MEMCACHED
   INJECTIONS <https://www.blackhat.com/us-14/briefings.html#the-new-page-of-injections-book-memcached-injections>`__.
-  Thanks to the PolarSSL project for the md5 implementation.
-  Thanks to `@lericson <https://github.com/lericson>`__ for the `benchmark
   script in
   pylibmc <https://github.com/lericson/pylibmc/blob/master/bin/runbench.py>`__.
-  Thanks to the libmemcached project and some other projects possibly
   not mentioned here.

Contributors
------------

-  `@mckelvin <https://github.com/mckelvin>`__
-  `@zzl0 <https://github.com/zzl0>`__
-  `@windreamer <https://github.com/windreamer>`__
-  `@lembacon <https://github.com/lembacon>`__
-  `@seansay <https://github.com/seansay>`__
-  `@mosasiru <https://github.com/mosasiru>`__
-  `@jumpeiMano <https://github.com/jumpeiMano>`__


Who is using
------------

- `豆瓣 <https://douban.com>`__
- `下厨房 <https://www.xiachufang.com>`__
- `Some other projects on GitHub <https://github.com/douban/libmc/network/dependents>`__
- Want to add your company/organization name here?
  Please feel free to send a PR!

Documentation
-------------

https://github.com/douban/libmc/wiki

LICENSE
-------

Copyright (c) 2014-2020, Douban Inc. All rights reserved.

Licensed under a BSD license:
https://github.com/douban/libmc/blob/master/LICENSE.txt

.. |build_go| image:: https://github.com/douban/libmc/actions/workflows/golang.yml/badge.svg
   :target: https://github.com/douban/libmc/actions/workflows/golang.yml

.. |build_py| image:: https://github.com/douban/libmc/actions/workflows/python.yml/badge.svg
   :target: https://github.com/douban/libmc/actions/workflows/python.yml

.. |pypiv| image:: https://img.shields.io/pypi/v/libmc
   :target: https://pypi.org/project/libmc/

.. |status| image:: https://img.shields.io/pypi/status/libmc
.. |pyversions| image:: https://img.shields.io/pypi/pyversions/libmc
.. |wheel| image:: https://img.shields.io/pypi/wheel/libmc
.. |license| image:: https://img.shields.io/pypi/l/libmc?color=blue
