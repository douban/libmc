#!/usr/bin/env python2
# from: https://gist.githubusercontent.com/pkrusche/7369262/raw/5bf2dc8afb88d3fdde7be6d16ee4290db6735f37/versioning.py

""" Git Versioning Script

Will transform stdin to expand some keywords with git version/author/date information.

Specify --clean to remove this information before commit.

Setup:

1. Copy versioning.py into your git repository

2. Run:

 git config filter.versioning.smudge 'python versioning.py'
 git config filter.versioning.clean  'python versioning.py --clean'
 echo 'version.py filter=versioning' >> .gitattributes
 git add versioning.py


3. add a version.py file with this contents:

 __version__ = ""
 __author__ = ""
 __email__ = ""
 __date__ = ""

or

 _Version = ""
 _Author = ""
 _Email = ""
 _Date = ""
"""

import sys
import subprocess
import re


def main():
    clean = False
    if len(sys.argv) > 1:
        if sys.argv[1] == '--clean':
            clean = True

    # initialise empty here. Otherwise: forkbomb through the git calls.
    subst_list = {
        "version": "",
        "date": "",
        # "author": "",
        # "email": ""
    }

    for line in sys.stdin:
        if not clean:
            subst_list = {
                # '--dirty' could be added to the following, too,
                # but is not supported everywhere
                "version": subprocess.check_output([
                    'git', 'describe', '--always', '--tags'
                ]),
                "date": subprocess.check_output([
                    'git', 'log', '--pretty=format:"%ad"', '-1'
                ]),
                # "author": subprocess.check_output([
                #     'git', 'log', '--pretty=format:"%an"', '-1'
                # ]),
                # "email": subprocess.check_output([
                #     'git', 'log', '--pretty=format:"%ae"', '-1'
                # ])
            }
            for k, v in subst_list.iteritems():
                v = re.sub(r'[\n\r\t"\']', "", v)
                rexp = "__%s__\s*=[\s'\"]+" % k
                line = re.sub(rexp, "__%s__ = \"%s\"\n" % (k, v), line)
                rexp = "_%s\s*=[\s'\"]+" % k.capitalize()
                line = re.sub(
                    rexp, "_%s = \"%s\"\n" % (k.capitalize(), v), line
                )
            sys.stdout.write(line)
        else:
            for k in subst_list:
                rexp = "__%s__\s*=.*" % k
                line = re.sub(rexp, "__%s__ = \"\"" % k, line)
                rexp = "_%s\s*=.*" % k.capitalize()
                line = re.sub(rexp, "_%s = \"\"" % k.capitalize(), line)
            sys.stdout.write(line)


if __name__ == "__main__":
    main()
