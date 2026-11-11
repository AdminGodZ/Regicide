#!/usr/bin/env bash
# Validation of r11f (lead 2026-11-11 14:32) on master fdd154e: DLL bl-fdd154e-r11f (fdd154e with book r11f, sha256
# a4c90ab6...; only src/weights/book.bin differs), runner and host from fdd154e (bin/match-fdd1).
#  a) targeted: ACF #6, Regicide Black, against KingsRow mode 3: 24 fresh, 24 kept (fixed colour), 24 natural pairs
#     (--pairs-together, our Black game first); concurrency 8, timing lock.     tags v11f-c6-<arm>
#  b) the new reference: three concurrency-1 runs exactly as mo2 (pairs together, kept processes, deck order,
#     --cb-claims --cb-move-limit 200), DLL bl-fdd154e-r11f, own host copies pinned by affinity_keeper.ps1; started as
#     three separate sessions (v11f_mo3_run.sh) after a).                            tags mo3-<opp>
# The WP12 c2 scoring is paused (SIGSTOP) from the start until all three b) runs have ended.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
pause() { for p in $(pgrep -f "wp12_select.py|blk-multiscore-1c6e"); do kill -$1 $p 2>/dev/null; done; }
pause STOP; st "V11F start (c2 scoring paused)"
export MATCH=$L/bin/match-fdd1 CBHOST=$L/bin/fdd1/cbhost.exe
D=regicide:bl-fdd154e-r11f
mkdir -p $L/wp11f
: > $L/wp11f/c6_b24.txt; for i in $(seq 24); do echo "| 9-13 22-18 10-14 | A=B" >> $L/wp11f/c6_b24.txt; done
: > $L/wp11f/c6_p24.txt; for i in $(seq 24); do echo "9-13 22-18 10-14" >> $L/wp11f/c6_p24.txt; done
for arm in fresh kept natural; do
  case $arm in
    fresh) X="--allow-rejected-options --fresh-engines"; DK=$L/wp11f/c6_b24.txt ;;
    kept) X="--allow-rejected-options"; DK=$L/wp11f/c6_b24.txt ;;
    natural) X="--allow-rejected-options --pairs-together"; DK=$L/wp11f/c6_p24.txt ;;
  esac
  pause STOP
  EXTRA_ARGS="$X" bash $J/bl_run_w.sh $D kingsrow v11f-c6-$arm $DK 0.5 8 < /dev/null || { st "V11F STOPPED at v11f-c6-$arm"; pause CONT; exit 1; }
done
st "V11F 2a done"
for o in kingsrow cake guinn; do bash $J/launch.sh mo3-$o $J/v11f_mo3_run.sh $o; done
until [ $(grep -c "^MO3 \(kingsrow\|cake\|guinn\) \(done\|STOPPED\)" $L/status) -ge 3 ]; do pause STOP; sleep 30; done
pause CONT; st "V11F done (c2 scoring resumed)"
