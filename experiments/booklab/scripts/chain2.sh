#!/usr/bin/env bash
# Chain after the book-mode finding (lead, 2026-11-10 20:00). Every run goes through bl_run_w.sh: explicit
# "set book 3" for every engine with a pre-flight read-back (a run whose read-back differs is refused), finished =
# exit 0 and all scheduled games. Order:
#   1. KingsRow in book mode 3 on the production deck: master + r10 + n6c (bl-456e9b2-r10), two runs of 312,
#      final-validation runner match-747ab51 (tags k3-kingsrow-{a,b}-{h1,h2})
#   2. review item C: runner accounting measurement (acct_run.sh, 3 x 20 games behind the logging shim)
#   3. WP10 block 4 (E vs EB on H2x; the part interrupted at 19:58 is replayed, and so is the finished
#      E-cake-H2x_0, which has no recorded exit status, so all of block 4 has the same settings)
#   4. WP11 validation: r11 with n6c, then n8q, acf156, match-747ab51 (tags v11-<net>-<opp>-<half>)
#   5. WP10 block 5 (EBN on H1, H2, H3)
#   6. WP10 block 7: EB against KingsRow in mode 3 on H1, H2, H3 (386 games; tags w10-b7-EB-kingsrow-<part>)
# A step whose run fails stops the chain (status line "CHAIN2 STOPPED").
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
fail() { st "CHAIN2 STOPPED at $1"; exit 1; }
st "CHAIN2 start"
for run in a b; do for h in h1 h2; do
  bash $J/bl_run_x.sh regicide:bl-456e9b2-r10 kingsrow k3-kingsrow-$run-$h $J/acf156_$h.txt < /dev/null || fail "k3-kingsrow-$run-$h"
done; done
st "K3 done"
bash $J/acct_run.sh < /dev/null > /dev/null 2>&1 || st "ACCT failed (chain continues)"
bash $J/wp10_blocks.sh 4 < /dev/null || fail "block 4"
grep -q "BL INCOMPLETE w10-b4\|BL refused w10-b4" $L/status && fail "block 4 (a run incomplete or refused)"
st "WP11 validation start"
for net in n6c n8q; do
  EA=""; [ $net = n8q ] && EA='set evalfile C:\regicide-match\nets\booklab\n8q.nnue'
  for h in h1 h2; do for opp in cake kingsrow guinn; do
    EXTRA_A="$EA" bash $J/bl_run_x.sh regicide:bl-456e9b2-r11 $opp v11-$net-$opp-$h $J/acf156_$h.txt < /dev/null \
      || fail "v11-$net-$opp-$h"
  done; done
  st "WP11 validation $net done"
done
bash $J/wp10_blocks.sh 5 < /dev/null || fail "block 5"
grep -q "BL INCOMPLETE w10-b5\|BL refused w10-b5" $L/status && fail "block 5 (a run incomplete or refused)"
st "WP10 block 7 start (EB vs KingsRow, book mode 3)"
export MATCH=$L/bin/match-cur EXTRA_ARGS=--allow-rejected-options
for part in H1 H2 H3_0 H3_1; do
  bash $J/bl_run_w.sh regicide:bl-456e9b2-r10 kingsrow w10-b7-EB-kingsrow-$part $L/wp10/parts/$part.txt 0.5 8 < /dev/null \
    || fail "w10-b7-EB-kingsrow-$part"
done
st "WP10 block 7 done"
st "CHAIN2 done"
