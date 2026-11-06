#!/usr/bin/env bash
# Cake 1.22, the GPL fork of Martin Fierz's Cake 1.4 (from the "capers" project), with its
# opening book (xbook.bin) and the Chinook 4-piece endgame database (db4).
# Output: build/opponents/cake/cake.dylib plus data files.
#
# Patches (cake/cake.patch):
#  - db.c/db.h: the Chinook db code assumed 32-bit 'long'; made LP64-clean and re-enabled the
#    endgame database on 64-bit targets (upstream 1.22 simply disabled it there).
#  - dll.c: write the new position back into the board (CB API contract; upstream only filled
#    the CBmove struct), honour CB_RESET_MOVES, and no logging to stdout.
#  - clock() is mapped to a wall clock by compat/cbport.h (CPU time on POSIX).
ENGINE_NAME=cake
source "$(dirname "$0")/common.sh"

CAPERS_COMMIT=90cf1769b8ae935e304994f0f3155c218c42ce39
fetch capers https://github.com/hungerburg/capers.git "$CAPERS_COMMIT"
apply_patch capers "$OPP_DIR/cake/cake.patch"
make_compat

src="$SRC/capers/cake"
dest="$OUT/cake"
mkdir -p "$dest"
log "compiling"
# -fcommon: dll.c and cakepp.c both define 'logging' (pre-C99 tentative definitions).
$CC -std=gnu99 $OPT_FLAGS -w -fPIC -fcommon -I"$COMPAT" -include "$OPP_DIR/compat/cbport.h" \
  -DSYS_UNIX -DVERSION=1.22 -DDBINI='"db.ini"' -DXBOOK='"xbook.bin"' \
  "$src/cakepp.c" "$src/db.c" "$src/movegen.c" "$src/book.c" "$src/dll.c" \
  $(dylib_flags cake.dylib) -o "$dest/cake.dylib"
cp "$src/xbook.bin" "$src/db4" "$src/db4.idx" "$src/db.ini" "$dest/"
log "built $dest/cake.dylib"
