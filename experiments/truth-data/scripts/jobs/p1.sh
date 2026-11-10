#!/usr/bin/env bash
# P1: n8bn vs Cake (x2) and KingsRow, config A, book off; plus n6c vs KingsRow in config A as the reference
# (the ponder agent's n6c config-A runs exist only vs Cake: pt-s2-a1-cake, pt-s2-a2-cake).
R=/mnt/c/regicide-match/jobs/truth/run_cb.sh
bash $R n8bn-cake1 cake C:/regicide-match/nets/n8bn.nnue
bash $R n8bn-cake2 cake C:/regicide-match/nets/n8bn.nnue
bash $R n8bn-kingsrow kingsrow C:/regicide-match/nets/n8bn.nnue
bash $R n6c-kingsrow kingsrow builtin
source /mnt/c/regicide-match/jobs/truth/common.sh; st "P1_DONE"
