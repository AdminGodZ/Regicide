# truth fens and truth pilot must reject inputs without a complete 16-byte record (an empty file, a 15-byte file) with a
# message and exit status 1, before they need the database (regression test: the fens guard of PR #41 was lost once in
# a merge; pilot's check came with PR #53).
#   cmake -DTRUTH=<path to truth> -DWORK=<scratch dir> -P truth_fens_test.cmake
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/empty.bin" "")
file(WRITE "${WORK}/short.bin" "123456789012345")
foreach(cmd fens pilot)
  foreach(name empty short)
    if(cmd STREQUAL "fens")
      set(args fens --egdb "${WORK}/no-such-egdb" --in "${WORK}/${name}.bin" --n 1)
    else()
      set(args pilot --egdb "${WORK}/no-such-egdb" --n 1 --threads 1 --out "${WORK}/pilot.csv" "${WORK}/${name}.bin")
    endif()
    execute_process(COMMAND "${TRUTH}" ${args} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc EQUAL 1)
      message(FATAL_ERROR "truth ${cmd} on ${name}.bin: exit status ${rc}, expected 1 (${err})")
    endif()
    if(NOT err MATCHES "holds no complete record")
      message(FATAL_ERROR "truth ${cmd} on ${name}.bin: unexpected message: ${err}")
    endif()
  endforeach()
endforeach()
message(STATUS "truth fens and truth pilot reject inputs without a complete record")
