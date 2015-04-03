#!/bin/sh
# ln -sf ../../misc/update_version.sh .git/hooks/pre-commit
VERSIONING_SCRIPT="`pwd`/misc/versioning.py"
VERSIONING_FILE="`pwd`/libmc/__init__.py"
TMPFILE=$VERSIONING_FILE".2"
cat $VERSIONING_FILE | python $VERSIONING_SCRIPT --clean | python $VERSIONING_SCRIPT > $TMPFILE
mv $TMPFILE $VERSIONING_FILE
git add $VERSIONING_FILE
