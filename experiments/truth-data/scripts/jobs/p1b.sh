#!/usr/bin/env bash
# Lead, 2026-11-10: n8bn and n8c against Cake and GuiNN on a DLL with the conversion search (master 2efe11a merged:
# truth-c0dde2e), config A, book off, 0.5 s/move, 312 games each. n8c's runs wait for its passed export check.
R=/mnt/c/regicide-match/jobs/truth/run_cb.sh; export DLLDIR=truth-c0dde2e
source /mnt/c/regicide-match/jobs/truth/common.sh
bash $R n8bn-cake-c0dde2e cake C:/regicide-match/nets/n8bn.nnue
bash $R n8bn-guinn-c0dde2e guinn C:/regicide-match/nets/n8bn.nnue
until [ -s ~/regicide-truth-work/n8c.ok ]; do grep -q "CHAIN3 stopped" $J/status && { st "P1B: n8c not available"; exit 1; }; sleep 60; done
cp ~/regicide-truth-work/n8c.nnue /mnt/c/regicide-match/nets/n8c.nnue
bash $R n8c-cake-c0dde2e cake C:/regicide-match/nets/n8c.nnue
bash $R n8c-guinn-c0dde2e guinn C:/regicide-match/nets/n8c.nnue
st "P1B_DONE"
