# Shared helpers for the opponent build scripts. Source this file; do not run it.
# Upstream sources are cloned into build/opponents/src/<name> at a pinned commit and patched there;
# nothing from them is committed to this repository (Cake is GPL, GuiNN is CC BY-NC-SA).

set -euo pipefail

OPP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$OPP_DIR/../.." && pwd)"
OUT="${REGICIDE_OPPONENTS_OUT:-$ROOT/build/opponents}"
SRC="$OUT/src"
COMPAT="$OUT/compat"

CC="${CC:-clang}"
CXX="${CXX:-clang++}"
case "$(uname -m)" in
  arm64|aarch64) ARCH_FLAGS="-mcpu=native" ;;
  *) ARCH_FLAGS="-march=native" ;;
esac
OPT_FLAGS="${OPT_FLAGS:--O3 $ARCH_FLAGS -fno-strict-aliasing}"
# Engines are shared libraries: .dylib on macOS, .so on Linux (including WSL).
if [ "$(uname -s)" = Darwin ]; then LIBEXT=dylib; else LIBEXT=so; fi

log() { printf '\033[1m[%s]\033[0m %s\n' "${ENGINE_NAME:-opponents}" "$*"; }

# fetch <dir-name> <git-url> <commit>: clone (once) and reset to a clean checkout of <commit>.
fetch() {
  local name="$1" url="$2" commit="$3" dir="$SRC/$1"
  mkdir -p "$SRC"
  if [ ! -d "$dir/.git" ]; then
    log "cloning $url"
    git clone --quiet "$url" "$dir"
  fi
  if ! git -C "$dir" cat-file -e "$commit^{commit}" 2>/dev/null; then
    git -C "$dir" fetch --quiet origin
  fi
  git -C "$dir" -c advice.detachedHead=false checkout --quiet --force "$commit"
  git -C "$dir" reset --quiet --hard "$commit"
  git -C "$dir" clean -qfdx
}

# apply_patch <dir-name> <patch-file>
apply_patch() {
  log "applying $(basename "$2")"
  git -C "$SRC/$1" apply --whitespace=nowarn "$2"
}

# Stub Win32 headers that all forward to compat/cbport.h.
make_compat() {
  mkdir -p "$COMPAT"
  for h in windows.h Windows.h conio.h ShlObj.h Shlwapi.h intrin.h; do
    printf '#pragma once\n#include "%s/compat/cbport.h"\n' "$OPP_DIR" > "$COMPAT/$h"
  done
}

lib_flags() {
  if [ "$LIBEXT" = dylib ]; then echo "-dynamiclib -install_name @rpath/$1"; else echo "-shared -Wl,-soname,$1"; fi
}
