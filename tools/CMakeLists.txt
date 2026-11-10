# Benchmarking tools: CheckerBoard engine host and match runner.
# Independent of regicide_core on purpose (the referee must not share the engine's movegen).
# Also builds standalone: cmake -S tools -B build-tools

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.20)
  project(RegicideTools CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  endif()
  enable_testing()
endif()

add_library(cbt_common STATIC common/checkers.cpp)
target_include_directories(cbt_common PUBLIC common)
if(NOT MSVC)
  target_compile_options(cbt_common PUBLIC -Wall -Wextra)
endif()

# cbhost runs everywhere; on Windows it loads 64-bit CheckerBoard DLLs.
add_executable(cbhost cbhost/cbhost.cpp)
target_link_libraries(cbhost PRIVATE cbt_common ${CMAKE_DL_LIBS})

add_executable(referee_test common/referee_test.cpp)
target_link_libraries(referee_test PRIVATE cbt_common)
add_test(NAME referee COMMAND referee_test)

# The runner is POSIX (posix_spawn/poll); on Windows run it in WSL and point it at cbhost.exe
# with exec: engine specs.
if(NOT WIN32)
  find_package(Threads REQUIRED)
  add_executable(match
    match/main.cpp
    match/engine_process.cpp
    match/openings.cpp
    match/stats.cpp
  )
  target_link_libraries(match PRIVATE cbt_common Threads::Threads)

  # The runner's side of the protocol, against a scripted engine (the test executable itself).
  add_executable(engine_process_test match/engine_process_test.cpp match/engine_process.cpp)
  target_include_directories(engine_process_test PRIVATE match)
  target_link_libraries(engine_process_test PRIVATE cbt_common Threads::Threads)
  add_test(NAME match_rejected_options COMMAND engine_process_test rejected)
  add_test(NAME match_deadline COMMAND engine_process_test deadline)

  # The runner's adjudication, through the match executable with two scripted engines.
  add_executable(terminal_before_draw_test match/terminal_before_draw_test.cpp match/engine_process.cpp)
  target_include_directories(terminal_before_draw_test PRIVATE match)
  target_link_libraries(terminal_before_draw_test PRIVATE Threads::Threads)
  add_test(NAME match_terminal_before_draw COMMAND terminal_before_draw_test $<TARGET_FILE:match>)

  # bookgen, run as a process on a prepared book (needs the engine library: not in the standalone tools build).
  if(TARGET regicide_core AND TARGET bookgen)
    foreach(name bookgen_frontier bookgen_sound_score)
      add_executable(${name}_test ${CMAKE_SOURCE_DIR}/tests/${name}_test.cpp match/engine_process.cpp)
      target_include_directories(${name}_test PRIVATE match ${CMAKE_SOURCE_DIR}/tests)
      target_link_libraries(${name}_test PRIVATE regicide_core Threads::Threads)
      add_test(NAME ${name} COMMAND ${name}_test $<TARGET_FILE:bookgen>)
    endforeach()
  endif()
endif()
