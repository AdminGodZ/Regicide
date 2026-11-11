#!/usr/bin/env bash
# re-test of the Skull-Cracker repair (r6): the ballot twice with both colours vs GuiNN
J=/mnt/c/regicide-match/jobs/booklab
bash $J/bl_target.sh bl-r6 t6-r6-guinn guinn 2 "10-15 22-17 15-19"
sleep 3
