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
  src/tt.cpp
  src/bench.cpp
)
target_include_directories(regicide_core PUBLIC src)
# Trained evaluation weights are compiled in with #embed (standard in C23/C++26; a Clang extension in C++ today).
set_source_files_properties(src/eval.cpp PROPERTIES COMPILE_OPTIONS "-Wno-c23-extensions"
                            OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/src/weights/eval.bin)

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

add_executable(cb_api_test tests/cb_api_test.cpp)
target_link_libraries(cb_api_test PRIVATE regicide_core)
add_test(NAME cb_api COMMAND cb_api_test $<TARGET_FILE:regicide_cb>)

add_executable(selfplay tools/selfplay.cpp)
target_link_libraries(selfplay PRIVATE regicide_core)
