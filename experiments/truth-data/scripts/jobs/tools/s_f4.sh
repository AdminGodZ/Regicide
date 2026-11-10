cd ~/regicide-jobs/truth; for f in lw_bl-f4-cake-h1.tsv lw_bl-f4-cake-h2.tsv; do [ -f $f ] && cut -f4,5,9,10,11,15,16,22-27 $f | cut -c1-230; done
grep "losswalk bl-f4" status | cut -c1-80
