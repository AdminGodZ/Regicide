#!/bin/sh
# Cross-check Regicide's EGDB against the Chinook 2..4-piece WLD database shipped with Cake 1.22.
#
#   tools/chinook/xcheck.sh <capers/cake dir> <regicide egdb dir> [build dir]
#
# Cake's db.c (GPL) is never copied into this repository: it is patched for LP64 (Chinook's code
# assumes 32-bit longs) into a scratch directory and linked with the harness for local use only.
set -eu
CAKE=$(cd "$1" && pwd)
EGDB=$2
BUILD=${3:-$(cd "$(dirname "$0")/../.." && pwd)/build}
SRC=$(cd "$(dirname "$0")" && pwd)
WORK=$(mktemp -d "${TMPDIR:-/tmp}/chinook-xcheck.XXXXXX")
trap 'rm -rf "$WORK"' EXIT

# 32-bit long -> int, matching format specifiers.
perl -pe 's/\bunsigned long\s+int\b/unsigned int/g; s/\bunsigned long\b/unsigned int/g; s/\blong\s+int\b/int/g; s/\blong\b/int/g; s/%ld/%d/g' \
    "$CAKE/db.c" > "$WORK/db.c"
cp "$CAKE/db.h" "$CAKE/switches.h" "$WORK/"
sed -i.bak 's/long/int/g' "$WORK/db.h"
cc -std=gnu89 -O2 -w -DSYS_UNIX '-DDBINI="db.ini"' -c "$WORK/db.c" -o "$WORK/db.o"
c++ -std=c++2c -O2 -I"$SRC/../../src" "$SRC/xcheck.cpp" "$WORK/db.o" "$BUILD/libregicide_egdb.a" -o "$WORK/xcheck"
# db.c reads ./db.ini, db4 and db4.idx from the working directory.
cp "$CAKE/db4" "$CAKE/db4.idx" "$WORK/"
echo "db4 db4.idx 4 512" > "$WORK/db.ini"
cd "$WORK" && ./xcheck "$EGDB"
