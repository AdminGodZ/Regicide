#!/usr/bin/env bash
# tm-tune builds from master 9841716: Linux tree (regicide, libregicide_cb.so, tools/match, cbhost; ctest, bench) for
# self-play, and the Windows CB DLL with the 2-8 egdb (engines/regicide/9841716-egdb8) for the Cake run.
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
JT=~/regicide-jobs/tm; mkdir -p $JT/engines $JT/bin $JT/matches
SHA=b88c02e
SHA=$SHA OUT=$SHA-egdb8 JOBS=3 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
B=~/regicide-match/builds/$SHA-egdb8/build-linux
nice cmake --build $B -j 3 > /dev/null
echo "-- linux ctest"; nice ctest --test-dir $B -j 3 2>&1 | tail -n 3
cp $B/libregicide_cb.so $JT/engines/rb88c02e.so
[ -e $JT/engines/egdb ] || ln -s ~/regicide-data/egdb $JT/engines/egdb
cp $B/tools/match $JT/bin/match-b88c02e; cp $B/tools/cbhost $JT/bin/cbhost; cp $B/bookgen $JT/bin/bookgen-b88c02e; mkdir -p ~/regicide-jobs/deep/bin; cp $B/bookgen ~/regicide-jobs/deep/bin/bookgen-b88c02e
echo "-- set param through the Linux .so (cbhost)"
printf 'cmd get dbpieces\ncmd set param tm_drop=25,tm_drop_factor=200\ncmd set param tm_open_pieces=18,tm_open_factor=175\ncmd set param bogus=1\nquit\n' | $JT/bin/cbhost $JT/engines/rb88c02e.so 2>&1
echo "-- set param through the Windows DLL (cbhost.exe)"
printf 'cmd get dbpieces\ncmd set param tm_soft=80\nquit\n' | /mnt/c/regicide-match/bin/b71dfa9/cbhost.exe "C:\\regicide-match\\engines\\regicide\\$SHA-egdb8\\regicide_cb.dll" 2>&1 | tr -d '\r'
echo "TM_BUILD2_DONE $(date -Is)" >> ~/regicide-jobs/r4/status
