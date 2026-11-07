#!/usr/bin/env bash
# GuiNN Checkers 2.06 (Jonathan Kreuzer; fork maintained by Ed Gilbert), the neural-net engine,
# as a native arm64 CheckerBoard-API dylib with its shipped nets (Nets206.gnn), opening book
# (opening.gbk) and built-in 4-piece endgame database (database.jef).
# Output: build/opponents/guinn/guinn.dylib (.so on Linux) plus data files, and build/opponents/guinn/nn_check.
#
# Port (guinn/guinn.patch + guinn/port/):
#  - NeuralNet/mathSimd.h: NEON implementations with the exact integer semantics of the AVX2
#    code; x86 paths untouched (AVX2 only when the compiler targets it).
#  - Windows-only pieces (GUI thread, registry, KingsRow egdb driver) replaced by
#    port/posix_glue.cpp; the egdb hook loads GuiNN's own 4-piece database instead.
#  - Standard-C++ fixes MSVC tolerated (GNUC bit-scan branch, template instantiation order,
#    functional casts, aligned_alloc size).
#
# Set GUINN_CHECK=0 to skip the NN equivalence check (nn_check, and its x86 AVX2 twin under
# Rosetta when available).
ENGINE_NAME=guinn
source "$(dirname "$0")/common.sh"

GUINN_COMMIT=615336a9c8d0dcb480e3ba20eef10ebbd7b36097
fetch GuiNN_Checkers https://github.com/eygilbert/GuiNN_Checkers.git "$GUINN_COMMIT"
apply_patch GuiNN_Checkers "$OPP_DIR/guinn/guinn.patch"
make_compat

src="$SRC/GuiNN_Checkers/src"
if [ "$(uname -s)" = Linux ]; then
  # Case-sensitive file system: two includes differ in case from the files (MSVC doesn't care).
  ln -sfn NeuralNet "$src/neuralNet"
  ln -sfn moveGen.h "$src/movegen.h"
fi
dest="$OUT/guinn"
mkdir -p "$dest"
sources=(board checkersNN database edDatabase engine evaluate gui_cb_interface movegen openingBook search transcript)
files=()
for f in "${sources[@]}"; do files+=("$src/$f.cpp"); done
files+=("$src/NeuralNet/NeuralNet.cpp" "$OPP_DIR/guinn/port/posix_glue.cpp")
flags=(-std=c++17 -DNDEBUG -DCBPORT_NO_MINMAX -DCBPORT_KEEP_CLOCK -Wno-c++11-narrowing -Wno-register -w
       -I"$COMPAT" -I"$src" -I"$src/NeuralNet" -include "$src/cb_interface.h" -include "$OPP_DIR/guinn/port/exports.h")

log "compiling dylib"
$CXX "${flags[@]}" $OPT_FLAGS -fPIC "${files[@]}" $(lib_flags guinn.$LIBEXT) -o "$dest/guinn.$LIBEXT"
cp "$SRC/GuiNN_Checkers/bin/Nets206.gnn" "$SRC/GuiNN_Checkers/bin/opening.gbk" "$SRC/GuiNN_Checkers/bin/database.jef" "$dest/"
log "built $dest/guinn.$LIBEXT"

if [ "${GUINN_CHECK:-1}" != 0 ]; then
  log "NN check (arm64 build vs scalar reference)"
  $CXX "${flags[@]}" $OPT_FLAGS "${files[@]}" "$OPP_DIR/guinn/nn_check.cpp" -o "$dest/nn_check"
  "$dest/nn_check" "$dest" "$dest/nn_values_native.txt"
  if [ "$(uname -m)" = arm64 ] && arch -x86_64 /usr/bin/true 2>/dev/null; then
    log "NN check (original x86 AVX2 code under Rosetta)"
    $CXX "${flags[@]}" -O3 -arch x86_64 -mavx2 -mpopcnt "${files[@]}" "$OPP_DIR/guinn/nn_check.cpp" -o "$dest/nn_check_x86_avx2"
    arch -x86_64 "$dest/nn_check_x86_avx2" "$dest" "$dest/nn_values_x86_avx2.txt"
    if cmp -s "$dest/nn_values_native.txt" "$dest/nn_values_x86_avx2.txt"; then
      log "arm64 NEON evaluations identical to x86 AVX2 on $(wc -l < "$dest/nn_values_native.txt" | tr -d ' ') positions"
    else
      log "ERROR: arm64 and x86 AVX2 evaluations differ"; exit 1
    fi
  fi
fi
