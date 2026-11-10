set -e
cd ~/regicide-truth
cp /mnt/c/regicide-match/jobs/truth/tools/truth.cpp tools/truth.cpp
cp /mnt/c/regicide-match/jobs/truth/tools/truth_fens_test.cmake tests/truth_fens_test.cmake
grep -q truth_fens CMakeLists.txt || sed -i 's|^target_link_libraries(truth PRIVATE regicide_core)$|&\nadd_test(NAME truth_fens_no_record COMMAND ${CMAKE_COMMAND} -DTRUTH=$<TARGET_FILE:truth> -DWORK=${CMAKE_CURRENT_BINARY_DIR}/truth_fens_test -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/truth_fens_test.cmake)|' CMakeLists.txt
grep -n "truth_fens" CMakeLists.txt
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cmake -S . -B build > /dev/null
nice cmake --build build -j 3 2>&1 | grep -E " error|warning: " | grep -v "store.cpp\|reach.cpp" | head
(cd build && ctest -j 4 2>&1 | grep -E "truth_fens|tests passed|FAILED")
# and the guard is gone -> the test must fail: simulate by checking it reports correctly on a real file
./build/truth fens --egdb ~/regicide-data/egdb --in ~/regicide-data/truth/gen5a_n50k_e2.bin --n 2 --min 4 --max 6 | head -2
