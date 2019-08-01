from __future__ import print_function
import os
import re
import sys
import shlex
import pkg_resources
from glob import glob
from setuptools import setup, Extension
from setuptools.command.test import test as TestCommand


sources = (glob("src/*.cpp") +
           ["libmc/_client.pyx"])
include_dirs = ["include"]

COMPILER_FLAGS = ["-fno-strict-aliasing", "-fno-exceptions", "-fno-rtti",
                  "-Wall", "-DMC_USE_SMALL_VECTOR", "-O3", "-DNDEBUG"]


def is_installed(requirement):
    try:
        pkg_resources.require(requirement)
    except pkg_resources.ResolutionError:
        return False
    else:
        return True

# Resolving Cython dependency via 'setup_requires' requires setuptools >= 18.0:
# https://github.com/pypa/setuptools/commit/a811c089a4362c0dc6c4a84b708953dde9ebdbf8
setuptools_req = "setuptools >= 18.0"
if not is_installed(setuptools_req):
    import textwrap
    print(textwrap.dedent("""
        setuptools >= 18.0 is required, and the dependency cannot be
        automatically resolved with the version of setuptools that is
        currently installed (%s).

        you can upgrade setuptools:
        $ pip install -U setuptools
        """ % pkg_resources.get_distribution("setuptools").version),
        file=sys.stderr)
    exit(1)


def find_version(*file_paths):
    with open(os.path.join(*file_paths)) as fhandler:
        version_file = fhandler.read()
        version_match = re.search(r"^__VERSION__ = ['\"]([^'\"]*)['\"]",
                                  version_file, re.M)
    if version_match:
        return version_match.group(1)
    raise RuntimeError("Unable to find version string.")


class PyTest(TestCommand):
    user_options = [('pytest-args=', 'a', "Arguments to pass to py.test")]

    def initialize_options(self):
        TestCommand.initialize_options(self)
        self.pytest_args = 'tests'

    def finalize_options(self):
        TestCommand.finalize_options(self)
        self.test_args = []
        self.test_suite = True

    def run_tests(self):
        #import here, cause outside the eggs aren't loaded
        import pytest
        errno = pytest.main(shlex.split(self.pytest_args))
        sys.exit(errno)

setup(
    name="libmc",
    packages=["libmc"],
    version=find_version("libmc", "__init__.py"),
    license= "BSD License",
    description="Fast and light-weight memcached client for C++/Python",
    author="PAN, Myautsai",
    author_email="myautsai@gmail.com",
    url="https://github.com/douban/libmc",
    keywords=["memcached", "memcache", "client"],
    long_description=open("README.rst").read(),
    classifiers=[
        "Intended Audience :: Developers",
        "License :: OSI Approved :: BSD License",
        "Development Status :: 5 - Production/Stable",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Programming Language :: Python :: Implementation :: CPython",
        "Operating System :: POSIX :: Linux",
        "Topic :: Software Development :: Libraries"
    ],
    # Support for the basestring type is new in Cython 0.20.
    setup_requires=["Cython >= 0.20"],
    cmdclass={"test": PyTest},
    ext_modules=[
        Extension("libmc._client", sources, include_dirs=include_dirs,
                  language="c++", extra_compile_args=COMPILER_FLAGS)
    ],
    tests_require=[
        "pytest",
        "future",
        "numpy",
    ]
)
