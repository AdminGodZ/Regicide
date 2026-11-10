cd ~/regicide-match/results
for d in $(ls -d *cake* *guinn* 2>/dev/null); do
  [ -f $d/summary.txt ] || continue
  l=$(grep -h '^A:' $d/summary.txt | awk '{print $2,$3,$4}')
  bk=$(grep -o 'set book [01]' $d/*.log 2>/dev/null | head -1)
  echo "$d | $l | $(head -c 200 $d/summary.txt | grep -o 'A = [^,]*' | head -1)"
done | grep -iE "off|pt-s2-[abc][12]|tr-" 
