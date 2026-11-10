cp /mnt/c/regicide-match/jobs/truth/tools/truth.cpp ~/regicide-truth/tools/truth.cpp
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cd ~/regicide-truth && nice cmake --build build -j 2 --target truth 2>&1 | grep -E "error|warning: " | grep -v store.cpp | head
ls -la build/truth | awk '{print $6, $7, $8}'
