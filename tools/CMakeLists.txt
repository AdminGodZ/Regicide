# Benchmarking tools: CheckerBoard engine host and match runner.
# Independent of regicide_core on purpose (the referee must not share the engine's movegen).

add_library(cbt_common STATIC common/checkers.cpp)
target_include_directories(cbt_common PUBLIC common)
target_compile_options(cbt_common PUBLIC -Wall -Wextra)

add_executable(cbhost cbhost/cbhost.cpp)
target_link_libraries(cbhost PRIVATE cbt_common ${CMAKE_DL_LIBS})

find_package(Threads REQUIRED)
add_executable(match
  match/main.cpp
  match/engine_process.cpp
  match/openings.cpp
  match/stats.cpp
)
target_link_libraries(match PRIVATE cbt_common Threads::Threads)

add_executable(referee_test common/referee_test.cpp)
target_link_libraries(referee_test PRIVATE cbt_common)
add_test(NAME referee COMMAND referee_test)
