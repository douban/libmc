#!/bin/sh

DIR="$( dirname `readlink -f "${BASH_SOURCE[0]}"`)"
HOOK="$DIR/../../.git/hooks/pre-commit"

[[ ! -e "$HOOK" ]] && ln -s "$DIR/pre-commit.sh" "$HOOK"
git apply --cached "$DIR/debug.patch" -R --recount -q
exit 0
