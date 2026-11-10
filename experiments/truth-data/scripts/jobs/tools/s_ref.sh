ls -d ~/regicide-match/results/bl-f1-* 2>&1 | head
for d in ~/regicide-match/results/bl-f1-cake-*; do echo "$d: $(grep -c '^\[Event' $d/games.pdn) games; $(grep -m2 'ACF #37' $d/games.pdn | tr '\n' ' ')"; done
grep -m3 "Opening\|Black \"" ~/regicide-match/results/bl-f4-cake-h1/games.pdn
