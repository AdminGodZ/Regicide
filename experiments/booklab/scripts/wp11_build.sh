#!/usr/bin/env bash
# WP11: pins from the final validation games; book r11 = r10 + pins; checks
set -e
W=~/regicide-booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; J=/mnt/c/regicide-match/jobs/booklab
cp $J/src/tools/booklab/pinwalk.cpp $J/src/tools/booklab/pins.py $W/tools/booklab/
grep -q booklab_pinwalk $W/CMakeLists.txt || sed -i 's|^target_link_libraries(booklab_bookcmp PRIVATE regicide_core)$|target_link_libraries(booklab_bookcmp PRIVATE regicide_core)\nadd_executable(booklab_pinwalk tools/booklab/pinwalk.cpp)\ntarget_link_libraries(booklab_pinwalk PRIVATE regicide_core)|' $W/CMakeLists.txt
bash $J/build_tree.sh 4 > /dev/null
cd $L
python3 $W/tools/booklab/pins.py --runs $R/bl-f1-* --book book_r10.bin --pinwalk $W/build-linux/booklab_pinwalk \
  --egdb ~/regicide-data/egdb --out pins.txt --report pins.md
{ cat decisions_r10.txt; echo; echo "# === WP11 pins (pins.txt)"; cat pins.txt; } > decisions_r11.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11.txt --out book_r11.bin | tail -1
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11.txt --out /tmp/r11b.bin > /dev/null
cmp book_r11.bin /tmp/r11b.bin && echo "r11 rebuilds byte-identically"
sha256sum book_r11.bin
grep -vc "^#\|^$" decisions_r11.txt
