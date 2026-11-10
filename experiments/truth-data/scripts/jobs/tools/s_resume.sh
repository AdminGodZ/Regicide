bash /mnt/c/regicide-match/jobs/truth/launch.sh n9h_trigger.sh
bash /mnt/c/regicide-match/jobs/truth/launch.sh losswalk.sh
sleep 5; tail -n 3 ~/regicide-jobs/truth/status | cut -c1-120; ls ~/regicide-jobs/truth/lw_*.tsv | wc -l; pgrep -a truth-tr4 | cut -c1-90
