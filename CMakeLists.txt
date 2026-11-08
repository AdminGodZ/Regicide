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
  src/nnue.cpp
  src/search.cpp
  src/tt.cpp
  src/bench.cpp
  src/book.cpp
)
target_include_directories(regicide_core PUBLIC src)
# Evaluation feature set (see eval.hpp); the built-in weights match the default.
option(REGICIDE_EVAL_BANDS "Eval: two-row band patterns" ON)
target_compile_definitions(regicide_core PUBLIC REGICIDE_EVAL_BANDS=$<BOOL:${REGICIDE_EVAL_BANDS}>)
# Trained evaluation weights are compiled in with #embed (standard in C23/C++26; a Clang extension in C++ today).
# src/weights/net.bin is an optional NNUE network (RGNN); left empty, the linear eval is used.
# NNUE layer widths, to build for a net of another architecture (empty: the defaults in nnue.hpp, which the
# built-in net uses; any other width rejects it and falls back to the linear eval).
set(REGICIDE_NNUE_FT "" CACHE STRING "NNUE feature-transformer width (empty: 1024)")
set(REGICIDE_NNUE_L1 "" CACHE STRING "NNUE first hidden layer width (empty: 32)")
foreach(width FT L1)
  if(REGICIDE_NNUE_${width})
    target_compile_definitions(regicide_core PUBLIC REGICIDE_NNUE_${width}=${REGICIDE_NNUE_${width}})
    set(REGICIDE_NNUE_CUSTOM ON)
  endif()
endforeach()
set_source_files_properties(src/eval.cpp PROPERTIES COMPILE_OPTIONS "-Wno-c23-extensions"
                            OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/src/weights/eval.bin;${CMAKE_SOURCE_DIR}/src/weights/net.bin")
set_source_files_properties(src/book.cpp PROPERTIES COMPILE_OPTIONS "-Wno-c23-extensions"
                            OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/src/weights/book.bin)

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

# CheckerBoard engine (getmove / enginecommand), also used by the match runner.
add_library(regicide_cb SHARED src/cb_api.cpp)
target_link_libraries(regicide_cb PRIVATE regicide_core)
set_target_properties(regicide_cb PROPERTIES CXX_VISIBILITY_PRESET hidden POSITION_INDEPENDENT_CODE ON)
set_target_properties(regicide_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_executable(perft_test tests/perft_test.cpp)
target_link_libraries(perft_test PRIVATE regicide_core)
add_test(NAME perft COMMAND perft_test)

add_executable(eval_test tests/eval_test.cpp)
target_link_libraries(eval_test PRIVATE regicide_core)
add_test(NAME eval_builtin COMMAND eval_test)

add_executable(nnue_test tests/nnue_test.cpp)
target_link_libraries(nnue_test PRIVATE regicide_core)
# Re-run CMake when net.bin changes, so the test knows whether a net is embedded.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/weights/net.bin)
file(SIZE ${CMAKE_SOURCE_DIR}/src/weights/net.bin REGICIDE_NET_BYTES)
if(REGICIDE_NNUE_CUSTOM)
  set(REGICIDE_NET_BYTES 0)  # the built-in net does not fit, so the test expects it to be rejected
endif()
target_compile_definitions(nnue_test PRIVATE REGICIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                           REGICIDE_NET_BYTES=${REGICIDE_NET_BYTES})
add_test(NAME nnue COMMAND nnue_test)
# The same test for other net widths, each built from its own copy of the sources it needs.
if(NOT REGICIDE_NNUE_CUSTOM)
  # Widths other than the default, so the embedded net does not fit them (REGICIDE_NET_BYTES=0).
  foreach(variant "ft512;REGICIDE_NNUE_FT=512" "l1_64;REGICIDE_NNUE_L1=64")
    list(GET variant 0 name)
    list(GET variant 1 def)
    add_executable(nnue_test_${name} tests/nnue_test.cpp src/nnue.cpp src/eval.cpp src/position.cpp src/movegen.cpp)
    target_include_directories(nnue_test_${name} PRIVATE src)
    target_compile_options(nnue_test_${name} PRIVATE $<TARGET_PROPERTY:regicide_core,INTERFACE_COMPILE_OPTIONS>)
    target_compile_definitions(nnue_test_${name} PRIVATE $<TARGET_PROPERTY:regicide_core,INTERFACE_COMPILE_DEFINITIONS>
                               ${def} REGICIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}" REGICIDE_NET_BYTES=0)
    add_test(NAME nnue_${name} COMMAND nnue_test_${name})
  endforeach()
endif()

add_executable(cb_api_test tests/cb_api_test.cpp)
target_link_libraries(cb_api_test PRIVATE regicide_core)
target_compile_definitions(cb_api_test PRIVATE REGICIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME cb_api COMMAND cb_api_test $<TARGET_FILE:regicide_cb>)

add_executable(selfplay tools/selfplay.cpp)
target_link_libraries(selfplay PRIVATE regicide_core)

add_executable(datagen tools/datagen.cpp)
target_link_libraries(datagen PRIVATE regicide_core)

add_executable(train tools/train.cpp)
target_link_libraries(train PRIVATE regicide_core)

add_executable(bookgen tools/bookgen.cpp)
target_link_libraries(bookgen PRIVATE regicide_core)

# Endgame database: library, generator and tests (self-contained; depends only on types.hpp).
add_library(regicide_egdb STATIC
  src/egdb/store.cpp
  src/egdb/codec.cpp
  src/egdb/index.cpp
  src/egdb/build.cpp
  src/egdb/verify.cpp
  src/egdb/platform.cpp
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

# The search probes the endgame database.
set_target_properties(regicide_egdb PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(regicide_core PUBLIC regicide_egdb)

option(REGICIDE_TOOLS "Build the match tools (cbhost, match runner)" ON)
if(REGICIDE_TOOLS)
  add_subdirectory(tools)
endif()
