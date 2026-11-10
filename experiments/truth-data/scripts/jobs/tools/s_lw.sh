cd ~/regicide-jobs/truth; ls -la lw_* | awk '{print $5, $9}'
for f in lw_pt-s2-a1-cake.tsv lw_tr-n8bn-cake1.tsv; do cut -f3,4,6,7,8,9,10,11,12,13,14,17 $f | cut -c1-180; done
tail -n 3 losswalk.log
