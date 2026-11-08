#!/usr/bin/env bash
# Phase 2 of the t3/nnue-speed verification (only after phase 1 passed): native builds of the new code (FT 512,
# FT 1024, L1 64, FT 768), an exactness check for FT 768 (nn3e) vs 821c89e with FT=768, then round-3-style matches
# with datagen paused (it was paused by phase 1):
#   (a) new + nn2b vs old ae210fb + nn2b   (b) nn3c FT1024 vs nn2b   (c) nn3d L1 64 vs nn2b   (d) nn3e FT768 vs nn2b
J=~/regicide-jobs/r3; N=~/regicide-nsv; W=~/regicide-nsv-work; R3=~/regicide-nnue3-work
mkdir -p $W/engines $W/matches
st() { echo "$* $(date -Is)" | tee -a $J/status >> $W/status; }
SLICE=$R3/nsv_valid_100k.bin
NN2B=$N/experiments/nnue2/nn2b.nnue; NN3C=$N/experiments/nnue3/nn3c.nnue; NN3D=$N/experiments/nnue3/nn3d.nnue; NN3E=$R3/nn3e.nnue
st "NSV phase 2 start $(git -C $N log --oneline -1)"
b() {  # b DIR [cmake args]
  local d=$1; shift
  cmake -S $N -B $N/$d -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "$@" > $W/$d.log 2>&1
  cmake --build $N/$d -j 16 >> $W/$d.log 2>&1 || { st "NSV build $d FAILED"; tail -n 30 $W/$d.log; exit 1; }
}
b build-m512
b build-m1024 -DREGICIDE_NNUE_FT=1024
b build-ml164 -DREGICIDE_NNUE_L1=64
b build-m768 -DREGICIDE_NNUE_FT=768
{
echo "== match builds: bench"
echo "m512 built-in: $($N/build-m512/regicide bench | tail -n 1)"
for x in "m1024 $NN3C" "ml164 $NN3D" "m768 $NN3E"; do set -- $x
  echo "$1 $(basename $2): $(printf 'set evalfile %s\nbench\nquit\n' $2 | $N/build-$1/regicide | grep '^bench')"; done
echo "old ae210fb built-in: $(~/regicide-match/builds/ae210fb/build-linux/regicide bench | tail -n 1)"
echo "== FT 768: ctest (new code built with FT=768)"
ctest --test-dir $N/build-m768 2>&1 | tail -n 3
B=~/regicide-nsv-base-768
[ -d $B ] || { git -C $N worktree add -q --detach $B 821c89e && sed -i 's/^constexpr int FT = 512;/constexpr int FT = 768;/' $B/src/nnue.hpp; }
grep -n '^constexpr int FT ' $B/src/nnue.hpp
cmake -S $B -B $B/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DREGICIDE_TOOLS=OFF > $W/base768.log 2>&1 && cmake --build $B/build -j 16 --target regicide >> $W/base768.log 2>&1
$N/build-m768/regicide evaldata $NN3E $SLICE 100000 > $W/eval_new_768.txt 2>/dev/null
$B/build/regicide evaldata $NN3E $SLICE 100000 > $W/eval_base_768.txt 2>/dev/null
cmp -s $W/eval_new_768.txt $W/eval_base_768.txt && echo "768 evaldata: $(wc -l < $W/eval_new_768.txt) evals, identical" || echo "768 evaldata: DIFFERENT"
for i in 1 2 3; do
  echo "base_768 $(printf 'set evalfile %s\nbench\nquit\n' $NN3E | $B/build/regicide | grep '^bench')"
  echo "new_768  $(printf 'set evalfile %s\nbench\nquit\n' $NN3E | $N/build-m768/regicide | grep '^bench')"
done
} > $W/checks_768.txt 2>&1
cat $W/checks_768.txt >> $W/status
FT768_OK=1
grep -q '768 evaldata: .* identical' $W/checks_768.txt || FT768_OK=0
grep -q '100% tests passed' $W/checks_768.txt || FT768_OK=0
[ $(grep -E '^(base|new)_768' $W/checks_768.txt | grep -o 'bench: [0-9]* nodes' | sort -u | wc -l) = 1 ] || FT768_OK=0
st "NSV FT768 exact: $FT768_OK"
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp $N/build-m512/libregicide_cb.so $W/engines/new512.so
cp $N/build-m512/libregicide_cb.so $W/engines/new512_A.so
cp $N/build-m1024/libregicide_cb.so $W/engines/new1024.so
cp $N/build-ml164/libregicide_cb.so $W/engines/newl164.so
cp $N/build-m768/libregicide_cb.so $W/engines/new768.so
cp ~/regicide-match/builds/ae210fb/build-linux/libregicide_cb.so $W/engines/old_ae210fb.so
M=$J/bin/match-825f993
COMMON="--openings $HOME/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments"
OPT="set threads 1"; OPT2="set hashsize 128"; OPT3="set book 0"
match() {  # match NAME_A SO_A EVALFILE_A|- NAME_B SO_B
  local OUT=$W/matches/$1-vs-$4
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && return 0
  rm -rf $OUT
  local EA=(); [ "$3" != - ] && EA=(--option-a "set evalfile $3")
  st "NSV start match $1 vs $4 (A $(basename $2) ${3##*/}, B $(basename $5) built-in)"
  $M -a $2 -b $5 --name-a $1 --name-b $4 --option-a "$OPT" --option-a "$OPT2" --option-a "$OPT3" "${EA[@]}" \
     --option-b "$OPT" --option-b "$OPT2" --option-b "$OPT3" $COMMON -o $OUT > $OUT.log 2>&1
  st "NSV end match $1 vs $4 rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
}
match new-nn2b $W/engines/new512_A.so - old-nn2b $W/engines/old_ae210fb.so
match nn3c-new $W/engines/new1024.so $NN3C nn2b-new $W/engines/new512.so
match nn3d-new $W/engines/newl164.so $NN3D nn2b-new $W/engines/new512.so
[ $FT768_OK = 1 ] && match nn3e-new $W/engines/new768.so $NN3E nn2b-new $W/engines/new512.so
DG=$(cat $J/nsv_paused_pids 2>/dev/null)
[ -n "$DG" ] && kill -CONT $DG && st "NSV resumed datagen $DG"
rm -f $J/matchlock
st "NSV_MATCHES_DONE"
