cmake_minimum_required(VERSION 3.30)
project(Regicide VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(REGICIDE_NATIVE "Optimize for the host CPU (-march=native)" ON)
option(REGICIDE_LTO "Enable link-time optimization in Release builds" ON)

add_library(regicide_core STATIC
  src/position.cpp
  src/movegen.cpp
  src/perft.cpp
  src/eval.cpp
  src/search.cpp
)
target_include_directories(regicide_core PUBLIC src)

if(MSVC)
  target_compile_options(regicide_core PUBLIC /W4)
else()
  target_compile_options(regicide_core PUBLIC -Wall -Wextra -Wpedantic)
  if(REGICIDE_NATIVE)
    target_compile_options(regicide_core PUBLIC $<$<CONFIG:Release>:-march=native>)
  endif()
endif()

if(REGICIDE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
  if(ipo_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  endif()
endif()

add_executable(regicide src/main.cpp)
target_link_libraries(regicide PRIVATE regicide_core)

enable_testing()
add_executable(perft_test tests/perft_test.cpp)
target_link_libraries(perft_test PRIVATE regicide_core)
add_test(NAME perft COMMAND perft_test)

# Endgame database: library, generator and tests (self-contained; depends only on types.hpp).
add_library(regicide_egdb STATIC
  src/egdb/store.cpp
  src/egdb/codec.cpp
  src/egdb/index.cpp
  src/egdb/build.cpp
  src/egdb/verify.cpp
)
target_include_directories(regicide_egdb PUBLIC src)
if(NOT MSVC)
  target_compile_options(regicide_egdb PRIVATE -Wall -Wextra -Wpedantic $<$<CONFIG:Release>:-O3>)
  if(REGICIDE_NATIVE)
    target_compile_options(regicide_egdb PUBLIC $<$<CONFIG:Release>:-march=native>)
  endif()
endif()

add_executable(egdb_gen tools/egdb_gen.cpp)
target_link_libraries(egdb_gen PRIVATE regicide_egdb)

add_executable(egdb_test tests/egdb_test.cpp)
target_link_libraries(egdb_test PRIVATE regicide_egdb)
add_test(NAME egdb COMMAND egdb_test)
add_test(NAME egdb_data COMMAND egdb_test --data)
