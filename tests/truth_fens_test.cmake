# truth fens must reject an input without a complete 16-byte record (an empty file, a 15-byte file) with a message and
# exit status 1, before it needs the database (regression test: the guard of PR #41 was lost once in a merge).
#   cmake -DTRUTH=<path to truth> -DWORK=<scratch dir> -P truth_fens_test.cmake
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/empty.bin" "")
file(WRITE "${WORK}/short.bin" "123456789012345")
foreach(name empty short)
  execute_process(COMMAND "${TRUTH}" fens --egdb "${WORK}/no-such-egdb" --in "${WORK}/${name}.bin" --n 1
                  RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 1)
    message(FATAL_ERROR "truth fens on ${name}.bin: exit status ${rc}, expected 1 (${err})")
  endif()
  if(NOT err MATCHES "holds no complete record")
    message(FATAL_ERROR "truth fens on ${name}.bin: unexpected message: ${err}")
  endif()
endforeach()
message(STATUS "truth fens rejects inputs without a complete record")
