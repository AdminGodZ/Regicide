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
# NNUE layer widths come from the net's header; this caps the feature transformer (per point of view), which sets
# the size of the search's accumulators.
set(REGICIDE_NNUE_MAX_FT "" CACHE STRING "Widest NNUE feature transformer this build loads (empty: 1024)")
if(REGICIDE_NNUE_MAX_FT)
  target_compile_definitions(regicide_core PUBLIC REGICIDE_NNUE_MAX_FT=${REGICIDE_NNUE_MAX_FT})
endif()
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

add_executable(search_test tests/search_test.cpp)
target_link_libraries(search_test PRIVATE regicide_core)
add_test(NAME search COMMAND search_test)

add_executable(fen_test tests/fen_test.cpp)
target_link_libraries(fen_test PRIVATE regicide_core)
add_test(NAME fen COMMAND fen_test)

add_executable(ttmove_test tests/ttmove_test.cpp)
target_link_libraries(ttmove_test PRIVATE regicide_core)
add_test(NAME ttmove COMMAND ttmove_test)

add_executable(eval_test tests/eval_test.cpp)
target_link_libraries(eval_test PRIVATE regicide_core)
add_test(NAME eval_builtin COMMAND eval_test)

add_executable(nnue_test tests/nnue_test.cpp)
target_link_libraries(nnue_test PRIVATE regicide_core)
# Re-run CMake when net.bin changes, so the test knows whether a net is embedded.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/weights/net.bin)
file(SIZE ${CMAKE_SOURCE_DIR}/src/weights/net.bin REGICIDE_NET_BYTES)
target_compile_definitions(nnue_test PRIVATE REGICIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                           REGICIDE_NET_BYTES=${REGICIDE_NET_BYTES})
# Random nets of several architectures (version, ft, l1, l2, buckets); the first also checks loading and the
# built-in net.
add_test(NAME nnue COMMAND nnue_test)
add_test(NAME nnue_ft512 COMMAND nnue_test 1 512 32 32 8)
add_test(NAME nnue_l1_64 COMMAND nnue_test 1 768 64 32 8)
add_test(NAME nnue_v2 COMMAND nnue_test 2 512 32 32 8)
add_test(NAME nnue_v2_wide COMMAND nnue_test 2 1024 16 32 8)
add_test(NAME nnue_v2_shapes COMMAND nnue_test 2 256 64 16 4)

add_executable(avoid_test tests/avoid_test.cpp)
target_link_libraries(avoid_test PRIVATE regicide_core)
add_test(NAME avoid COMMAND avoid_test)

add_executable(cb_api_test tests/cb_api_test.cpp)
target_link_libraries(cb_api_test PRIVATE regicide_core)
target_compile_definitions(cb_api_test PRIVATE REGICIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME cb_api COMMAND cb_api_test $<TARGET_FILE:regicide_cb>)

add_executable(selfplay tools/selfplay.cpp)
target_link_libraries(selfplay PRIVATE regicide_core)

add_executable(unbalanced tools/unbalanced.cpp)
target_link_libraries(unbalanced PRIVATE regicide_core)

add_executable(posbench tools/posbench.cpp)
target_link_libraries(posbench PRIVATE regicide_core)

add_executable(datagen tools/datagen.cpp)
target_link_libraries(datagen PRIVATE regicide_core)

add_executable(train tools/train.cpp)
target_link_libraries(train PRIVATE regicide_core)

add_executable(bookgen tools/bookgen.cpp)
target_link_libraries(bookgen PRIVATE regicide_core)

add_executable(lossprobe tools/lossprobe.cpp)
target_link_libraries(lossprobe PRIVATE regicide_core)

# booklab (tools/booklab): theory-table replay and decisions-to-book builder.
add_executable(booklab_replay tools/booklab/replay.cpp)
target_link_libraries(booklab_replay PRIVATE regicide_core)
add_executable(booklab_apply tools/booklab/bookapply.cpp)
target_link_libraries(booklab_apply PRIVATE regicide_core)
add_executable(booklab_show tools/booklab/bookshow.cpp)
target_link_libraries(booklab_show PRIVATE regicide_core)
add_executable(booklab_dbscan tools/booklab/dbscan.cpp)
target_link_libraries(booklab_dbscan PRIVATE regicide_core)
add_executable(booklab_reach tools/booklab/reach.cpp)
target_link_libraries(booklab_reach PRIVATE regicide_core)
add_executable(booklab_bookcmp tools/booklab/bookcmp.cpp)
target_link_libraries(booklab_bookcmp PRIVATE regicide_core)

add_executable(truth tools/truth.cpp)
target_link_libraries(truth PRIVATE regicide_core)

add_executable(rescore tools/rescore.cpp)
target_link_libraries(rescore PRIVATE regicide_core)

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

# Conversion of database wins without distance information, on a small generated database.
add_executable(conversion_test tests/conversion_test.cpp)
target_link_libraries(conversion_test PRIVATE regicide_core)
add_test(NAME conversion COMMAND conversion_test)

# Regressions for review fixes: one executable each, two of them with one ctest entry per protected behaviour.
# The ones that need an endgame database build a small one in a temporary directory (tests/test_egdb.hpp).
foreach(name lmr_isolation hash_clear book_king_run egdb_root nnue_bounds sound_score fixed_movetime
             illegal_book_entry count_key_boundaries etc_repetition conversion_lmp_disabled near_limit_pruning)
  add_executable(${name}_test tests/${name}_test.cpp)
  target_link_libraries(${name}_test PRIVATE regicide_core)
endforeach()
add_test(NAME lmr_isolation COMMAND lmr_isolation_test)
add_test(NAME hash_clear_evaluator COMMAND hash_clear_test evaluator)
add_test(NAME hash_clear_egdb COMMAND hash_clear_test egdb)
add_test(NAME hash_resize_refused COMMAND hash_clear_test resize)
add_test(NAME threads_refused COMMAND hash_clear_test threads)
add_test(NAME book_king_run COMMAND book_king_run_test)
add_test(NAME egdb_root COMMAND egdb_root_test)
add_test(NAME nnue_bounds COMMAND nnue_bounds_test)
add_test(NAME sound_score COMMAND sound_score_test)
add_test(NAME fixed_movetime COMMAND fixed_movetime_test)
add_test(NAME illegal_book_entry COMMAND illegal_book_entry_test)
add_test(NAME count_key_boundaries COMMAND count_key_boundaries_test)
add_test(NAME etc_repetition COMMAND etc_repetition_test)
add_test(NAME conversion_lmp_disabled COMMAND conversion_lmp_disabled_test)
add_test(NAME limit_rfp COMMAND near_limit_pruning_test rfp)
add_test(NAME limit_probcut COMMAND near_limit_pruning_test probcut)
add_test(NAME limit_singular COMMAND near_limit_pruning_test singular)

option(REGICIDE_TOOLS "Build the match tools (cbhost, match runner)" ON)
if(REGICIDE_TOOLS)
  add_subdirectory(tools)
endif()
