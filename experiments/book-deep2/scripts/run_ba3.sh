#!/usr/bin/env bash
# One CheckerBoard run (book/avoid tests; run_ba2.sh plus OPENINGS) with the baseline settings (acf156, 312 games, -c 8, hash 128,
# threads 1, Regicide egdb via the DLL's egdb/ folder; opponents with their books and the KingsRow 2-8 WLD db).
#   run_ba3.sh DLLDIR TAG OPP MOVETIME [CMD...]   (env OPENINGS: deck or file, default acf156)   (each non-empty CMD is one engine command for Regicide)
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/ba-TAG-OPP.
set -u
DLLDIR=$1; TAG=$2; OPP=$3; MT=$4; shift 4
MATCH=~/regicide-match/bin/match-747ab51
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
REG="exec:$CB \"$E\\regicide\\$DLLDIR\\regicide_cb.dll\""
DBO=(--option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" --option-b "set max_dbpieces 8" --option-b "set hashsize 128")
case $OPP in
  kingsrow*) DLL="$E\kingsrow\Kingsrow64.dll"; OX=(--option-b "set searchthreads 1") ;;
  cake)      DLL="$E\cake\cake_189f.dll"; OX=() ;;
  guinn)     DLL="$E\guinn\gui_nn64.dll"; OX=() ;;
esac
AX=(); DESC=""
for c in "$@"; do [ -n "$c" ] && { AX+=(--option-a "$c"); DESC="$DESC '$c'"; }; done
L=~/regicide-jobs/r4; OUT=~/regicide-match/results/ba-$TAG-$OPP
st() { echo "$* $(date -Is)" >> $L/status; }
if [ -f "$OUT/summary.txt" ] && grep -q "^Elo (pent" "$OUT/summary.txt"; then st "BA skip $TAG $OPP"; exit 0; fi
rm -rf "$OUT"
st "BA start $TAG $OPP movetime $MT openings ${OPENINGS:-acf156} dll $DLLDIR${DESC:+ option-a$DESC}; bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
"$MATCH" -a "$REG" -b "exec:$CB \"$DLL\"" \
  --option-a "set hashsize 128" --option-a "set threads 1" "${AX[@]}" \
  "${DBO[@]}" "${OX[@]}" \
  --openings ${OPENINGS:-acf156} --movetime $MT -c 8 --pdn-comments -o "$OUT" > "$L/ba-$TAG-$OPP.log" 2>&1
st "BA end $TAG $OPP rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
