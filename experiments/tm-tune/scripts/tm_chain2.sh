#!/usr/bin/env bash
# Run inside an attached wsl.exe. Time-management tuning on master b88c02e, after the Cake analysis.
#  1. Self-play at 0.1 s/move (CheckerBoard banked movetime, the tools/match default), 805 openings x 2, -c 10,
#     book off, 2-8 EGDB, both sides rb88c02e.so; A = "set param <variant>", B = default. Datagen paused (matchlock).
#  2. Variants with CI lower bound > 0 again at 0.3 s/move.
#  3. Cake at 0.5 s (acf156, 312 games; DLL b88c02e-egdb8, 2-8 EGDB, no book/avoid; gen5a alongside):
#     off (same DLL), the best tm_open variant (T5/T6, by 0.1 s Elo) regardless of its self-play result, and the best
#     variant confirmed at 0.3 s (CI lower bound > 0) if it is a different one.
J=~/regicide-jobs/r4; JT=~/regicide-jobs/tm; R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh
st() { echo "TM $* $(date -Is)" >> $J/status; }
declare -A V=([T1]="tm_drop=25,tm_drop_factor=200" [T2]="tm_drop=50,tm_drop_factor=250" [T3]="tm_soft=80" [T4]="tm_soft=45"
              [T5]="tm_open_pieces=18,tm_open_factor=175" [T6]="tm_open_pieces=20,tm_open_factor=250")
until grep -q '^CAKE_ALL_DONE' $J/status; do sleep 30; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
touch $J/matchlock; sleep 8
st "chain2 start (b88c02e); matchlock taken: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
match() {  # match NAME MOVETIME
  local n=$1 mt=$2 OUT=$JT/matches/$1-$2s
  grep -q '^Elo (pent' $OUT/summary.txt 2>/dev/null && return 0
  rm -rf $OUT
  st "start $n (${V[$n]}) vs default at $mt s/move"
  $JT/bin/match-b88c02e --cbhost $JT/bin/cbhost -a $JT/engines/rb88c02e.so -b $JT/engines/rb88c02e.so --name-a $n --name-b default \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set param ${V[$n]}" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime $mt -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end $n $mt s rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
}
lo() { grep '^Elo (pent' $JT/matches/$1/summary.txt | sed -E 's/^Elo \(pentanomial, pairs\): [-+0-9.]+ \[([-+0-9.]+), .*/\1/'; }
el() { grep '^Elo (pent' $JT/matches/$1/summary.txt | sed -E 's/^Elo \(pentanomial, pairs\): ([-+0-9.]+) .*/\1/'; }
for n in T1 T2 T3 T4 T5 T6; do match $n 0.1; done
st "STAGE1_DONE"
CONF=""; BEST=""; BESTEL=-999
for n in T1 T2 T3 T4 T5 T6; do
  if awk "BEGIN{exit !($(lo $n-0.1s) > 0)}"; then
    match $n 0.3
    if awk "BEGIN{exit !($(lo $n-0.3s) > 0)}"; then
      CONF="$CONF $n"
      awk "BEGIN{exit !($(el $n-0.3s) > $BESTEL)}" && { BEST=$n; BESTEL=$(el $n-0.3s); }
    fi
  fi
done
OPEN=T5; awk "BEGIN{exit !($(el T6-0.1s) > $(el T5-0.1s))}" && OPEN=T6
st "STAGE2_DONE confirmed:${CONF:- none} best:${BEST:-none}; best tm_open variant at 0.1 s: $OPEN"
rm -f $J/matchlock
bash $R b88c02e-egdb8 tm-off cake 0.5
bash $R b88c02e-egdb8 tm-$OPEN cake 0.5 "set param ${V[$OPEN]}"
[ -n "$BEST" ] && [ "$BEST" != "$OPEN" ] && bash $R b88c02e-egdb8 tm-$BEST cake 0.5 "set param ${V[$BEST]}"
st "TM_ALL_DONE"
