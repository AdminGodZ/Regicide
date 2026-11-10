#!/usr/bin/env bash
# Opponent configuration evidence (log: experiments/booklab/data/engine_probe.log). Same option lists as bl_run.sh.
P=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/engine_probe.py
CB=/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
O=~/regicide-jobs/booklab/engine_probe.log
FEN8="W:W17,21,22,25:B5,9,10,K14"   # 8 pieces, endgame-database range
GETS=(--get "get book" --get "get dbpath" --get "get hashsize" --get "get max_dbpieces" --get "get enable_wld"
      --get "get dbmbytes" --get "get searchthreads" --get "get gamehist" --get "name" --get "about" --get "help")
OPP=(--opt "set dbpath $DB8" --opt "set enable_wld 1" --opt "set dbmbytes 512" --opt "set max_dbpieces 8" --opt "set hashsize 128")
{
  echo "# engine probe $(date -Is); cbhost b71dfa9; options exactly as the match runs send them (bl_run.sh)"
  echo; echo "## Cake ($E\\cake\\cake_189f.dll)"
  python3 $P --cbhost $CB --dll "$E\\cake\\cake_189f.dll" "${OPP[@]}" "${GETS[@]}" --fen "$FEN8"
  echo; echo "## KingsRow ($E\\kingsrow\\Kingsrow64.dll)"
  python3 $P --cbhost $CB --dll "$E\\kingsrow\\Kingsrow64.dll" "${OPP[@]}" --opt "set searchthreads 1" "${GETS[@]}" --fen "$FEN8"
  echo; echo "## GuiNN ($E\\guinn\\gui_nn64.dll)"
  python3 $P --cbhost $CB --dll "$E\\guinn\\gui_nn64.dll" "${OPP[@]}" "${GETS[@]}" --fen "$FEN8"
  echo; echo "## Regicide bl-final1 ($E\\regicide\\bl-final1\\regicide_cb.dll)"
  python3 $P --cbhost $CB --dll "$E\\regicide\\bl-final1\\regicide_cb.dll" --opt "set hashsize 128" --opt "set threads 1" \
    --get "get book" --get "get avoid" --get "get dbpath" --get "get dbpieces" --get "get hashsize" --get "get threads" \
    --get "get evalfile" --get "name" --get "about" --fen "$FEN8"
} > $O 2>&1
echo "probe written: $O"
