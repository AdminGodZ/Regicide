#!/usr/bin/env bash
# n9h_restart.sh: once n9.sh n9h has taken and deduplicated its E5 snapshot, restart both generators with 6-16 random
# plies for the random-opening half (lead, 2026-11-11: from the next restart; the n9h snapshot is that point), seeds
# 620 / 621, the same verified binary. Offsets of the change: restart_e5.log / restart_offset_*.txt.
source /mnt/c/regicide-match/jobs/truth/common.sh
st "n9h_restart armed (6-16 random plies after the n9h snapshot)"
until grep -q "TR n9h: E5 deduplicated" $J/status; do sleep 30; done
RANDOM_PLIES=6-16 bash /mnt/c/regicide-match/jobs/truth/restart_e5.sh a99a1334887d2434 620 621
