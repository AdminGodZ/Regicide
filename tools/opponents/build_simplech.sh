#!/usr/bin/env bash
# Simple Checkers 1.15 (Martin Fierz), the sample engine shipped with CheckerBoard. A weak
# sanity opponent. Output: build/opponents/simplech/simplech.dylib (.so on Linux)
ENGINE_NAME=simplech
source "$(dirname "$0")/common.sh"

CB_COMMIT=32f194f840f208f0aff86a07026f788c01c819ce
fetch CheckerBoard https://github.com/eygilbert/CheckerBoard.git "$CB_COMMIT"
apply_patch CheckerBoard "$OPP_DIR/simplech/simplech.patch"
make_compat

dest="$OUT/simplech"
mkdir -p "$dest"
log "compiling"
$CXX -std=c++17 $OPT_FLAGS -w -fPIC -I"$COMPAT" -I"$SRC/CheckerBoard/source" \
  -include "$OPP_DIR/compat/cbport.h" -include "$OPP_DIR/simplech/exports.h" \
  -x c++ "$SRC/CheckerBoard/source/simplech.c" $(lib_flags simplech.$LIBEXT) -o "$dest/simplech.$LIBEXT"
log "built $dest/simplech.$LIBEXT"
