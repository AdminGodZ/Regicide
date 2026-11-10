# Building the production CheckerBoard DLL (`regicide_cb.dll`)

All match DLLs of the booklab work (`bl-456e9b2-r10` = master + book r10, the merged production engine;
`bl-456e9b2-r11`; the WP10 configurations) are cross-compiled on Win11-AdminGod under WSL2 (Ubuntu 22.04) with the
files in this directory. The script that does it is `experiments/booklab/scripts/bl_build_dll.sh`; the commands
below are what it runs.

## Toolchain

| Tool | Version |
|---|---|
| llvm-mingw | `llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64` (https://github.com/mstorsjo/llvm-mingw/releases), UCRT variant |
| compiler | clang 23.1.5 (llvm-project 392dfff97d29b59d77e5cac6d4b6ef2585fe7117), target `x86_64-w64-windows-gnu` |
| C++ runtime | libc++ / libunwind / compiler-rt of that llvm-mingw, linked statically |
| CMake / Ninja | 4.4.5 / 1.11.1 |

`mingw-clang.cmake` takes the toolchain directory from the environment variable `LLVM_MINGW`.

## Configure and build

```sh
export LLVM_MINGW=/path/to/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64
git worktree add --detach src 456e9b2              # engine source (456e9b2 = master with PR #39)
cp book_r10.bin src/src/weights/book.bin           # the book is compiled in (see "Run time" below)
cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=/path/to/t3-booklab/tools/windows/mingw-clang.cmake   # this directory's file
cmake --build build --target regicide_cb
cp build/libregicide_cb.dll <engine folder>/regicide_cb.dll
```

Flags come from the engine's `CMakeLists.txt` with its defaults:

- `CMAKE_BUILD_TYPE=Release`: `-O3 -DNDEBUG`.
- `REGICIDE_NATIVE=ON` (default): `-march=native`. On the build machine (Intel Core Ultra 9 285K) clang resolves
  this to `-target-cpu arrowlake-s`: AVX2, AVX-VNNI, BMI2, POPCNT, no AVX-512. **The production DLL therefore
  needs a CPU with these extensions.** A portable build: `-DREGICIDE_NATIVE=OFF` (plus e.g.
  `-DCMAKE_CXX_FLAGS=-march=x86-64-v3`); no match of this work used one.
- `REGICIDE_LTO=ON` (default): link-time optimisation (`CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE`).
- Linking: `-static` (from `mingw-clang.cmake`), so the C++ runtime is inside the DLL. Its imports, as built:
  `KERNEL32.dll` and the Windows UCRT API sets `api-ms-win-crt-{runtime,convert,private,string,math,heap,stdio,
  time,locale,multibyte,environment,filesystem}-l1-1-0.dll` (part of Windows 10 and 11). No other DLL is needed.
- Before 456e9b2 some branches needed Windows portability patches (kept with the old run-vs.sh tooling); 456e9b2
  and later build as they are.

`bl_build_dll.sh NAME BOOK [JOBS [REF]]` does the above for REF (default the booklab HEAD) with BOOK, installs the
DLL as `C:\regicide-match\engines\regicide\bl-NAME\regicide_cb.dll`, adds the database junction (below), writes
`BUILD.txt` (engine commit, book sha256, compiler) and prints the Linux bench with the book on and off.

## Run time: where the net, book and database come from

- **Evaluation net:** compiled in with `#embed` from `src/weights/net.bin` (456e9b2: net n6c, sha256
  `6b335193073ef5f076e28b98407f75da194902705e9cf35b85f43155d29efc5e`), with the linear fallback `src/weights/eval.bin`.
  `set evalfile PATH` replaces it (used only for the n8q configurations: `C:\regicide-match\nets\booklab\n8q.nnue`);
  `get evalfile` reports what is loaded.
- **Opening book and avoid list:** compiled in with `#embed` from `src/weights/book.bin` (r10 sha256
  `cb4e2bb2aea4d6de6bc789513baa8b49ae70b06d5d37b064adbf1a6018197a1e`; r11 `2509043e0ee4ee60a89eda256f369892b67640a6b58ec3249dd7f7f329243b29`).
  Default book mode 3 (best move, margin 0); every match script now sends `set book 3` explicitly and checks the
  read-back. The avoid list is the embedded book's unless `set avoidfile PATH`.
- **Endgame database:** the folder `egdb` next to the DLL (the DLL's own path from `GetModuleFileName`, plus
  `\egdb`), opened at the first engine command with a 4096 MB cache (`set dbmbytes` changes the cache). It holds
  Regicide's own `.rgdb` files for 2 to 8 pieces (11 GB). The 11 GB are not copied per engine: each engine folder's
  `egdb` is a directory junction to `C:\regicide-match\engines\regicide\6bb8c6c-deep2b-egdb8\egdb`.
  `get dbpath` / `get dbpieces` report what is in use (8).
- Hash 128 MB by default (`set hashsize`), one search thread (`set threads 1`, sent by every match script).

## Is a rebuild the same binary?

Rebuilding `bl-456e9b2-r10` with these commands (2026-11-10, `bl_build_dll.sh repro-r10 book_r10.bin 2 456e9b2`)
gave a DLL of the same size whose code and data sections are byte-identical to the production DLL; only the link
timestamp, the `.buildid` section (PDB GUID) and the DWARF debug sections (which name the build directory) differ.
Check with `python3 tools/windows/pe_compare.py production.dll rebuilt.dll`.
