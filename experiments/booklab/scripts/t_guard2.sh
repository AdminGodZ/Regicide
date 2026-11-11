T=$(mktemp -d)
mk() { mkdir -p $T/$1; [ -n "$2" ] && echo $2 > $T/$1/exit_status; echo "Games: $3 of 156 scheduled" > $T/$1/summary.txt; return 0; }
mk ok 0 156; mk partial 0 1; mk old "" 156; mk oldpartial "" 1; mk failed 4 156
for n in ok partial old oldpartial failed; do
  d=$T/$n
  if { [ ! -f $d/exit_status ] || [ "$(cat $d/exit_status)" = 0 ]; } && grep -Eq "^Games: ([0-9]+) of \1 scheduled" $d/summary.txt; then echo "$n: accepted"; else echo "$n: refused"; fi
done
rm -rf $T
