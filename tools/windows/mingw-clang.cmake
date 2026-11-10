# CMake toolchain: cross-compile 64-bit Windows binaries from Linux/WSL with llvm-mingw
# (https://github.com/mstorsjo/llvm-mingw, ucrt variant). Clang is needed for the engine's #embed.
# Set LLVM_MINGW to the unpacked toolchain directory. See README.md in this directory for the exact
# commands that build the production CheckerBoard DLL.
#
# Everything is linked statically (-static), so the DLL only imports KERNEL32 and the Windows
# UCRT, which ship with Windows 10 and 11. -march=native in the engine's CMakeLists targets the
# CPU of the machine doing the build; under WSL that is the Windows machine itself.

if(NOT DEFINED ENV{LLVM_MINGW})
  message(FATAL_ERROR "set LLVM_MINGW to the llvm-mingw directory")
endif()
set(LLVM_MINGW $ENV{LLVM_MINGW})

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang)
set(CMAKE_CXX_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-clang++)
set(CMAKE_RC_COMPILER ${LLVM_MINGW}/bin/x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH ${LLVM_MINGW}/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static")
