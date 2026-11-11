# truth's PDN reader (used by losswalk for the in-game score): a {comment} inside a (variation) must not be attached to
# the last main-line move; main-line comments, also over two lines, stay attached (review r17, finding 3).
#   cmake -DTRUTH=<path to truth> -DWORK=<scratch dir> -P truth_pdn_test.cmake
file(MAKE_DIRECTORY "${WORK}")
file(WRITE "${WORK}/var.pdn" "[Event \"t\"]
[Result \"*\"]

1. 11-15 {score -12} 22-18 (22-17 {score -999} 9-13 {value=-888}) 15x22 {score
 -30 depth 9} 25x18 *
")
execute_process(COMMAND "${TRUTH}" pdn "${WORK}/var.pdn" RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "truth pdn: exit status ${rc} (${err})")
endif()
set(expected "1 1 11-15 {score -12}\n1 2 22-18 {}\n1 3 15x22 {score  -30 depth 9}\n1 4 25x18 {}\n")
if(NOT out STREQUAL expected)
  message(FATAL_ERROR "truth pdn: unexpected output:\n${out}expected:\n${expected}")
endif()
message(STATUS "truth's PDN reader keeps comments in variations off the main line")
