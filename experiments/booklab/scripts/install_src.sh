#!/usr/bin/env bash
# copy staged sources into the worktree and add the booklab targets to CMakeLists.txt (once)
set -e
W=~/regicide-booklab
mkdir -p $W/tools/booklab
cp /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/*.cpp $W/tools/booklab/
for f in /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/*.py /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/*.md; do [ -f "$f" ] && cp "$f" $W/tools/booklab/; done
if ! grep -q booklab_replay $W/CMakeLists.txt; then
  python3 - $W/CMakeLists.txt <<'PY'
import sys
p = sys.argv[1]; s = open(p).read()
old = "add_executable(lossprobe tools/lossprobe.cpp)\ntarget_link_libraries(lossprobe PRIVATE regicide_core)\n"
assert s.count(old) == 1
s = s.replace(old, old + "\n# booklab (tools/booklab): theory-table replay and decisions-to-book builder.\n"
  "add_executable(booklab_replay tools/booklab/replay.cpp)\ntarget_link_libraries(booklab_replay PRIVATE regicide_core)\n"
  "add_executable(booklab_apply tools/booklab/bookapply.cpp)\ntarget_link_libraries(booklab_apply PRIVATE regicide_core)\n")
open(p, "w").write(s)
PY
fi
echo installed
