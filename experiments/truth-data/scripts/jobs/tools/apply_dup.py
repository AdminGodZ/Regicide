import os
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
block = """### E5: repeated games (review round, 2026-11-10 evening)

**Finding.** A duplicate-game check (`scripts/dupgames.py`: a game = the block from one GAME_START to the next;
duplicate = byte-identical block) found many copies:
- **gen6a:** 34,191 of 149,941 games;
- **gen6b:** 28,546 of 148,139 games;
- 12,515 games in both files.

**Cause and fix:**
- **Not restart replays.** gen6a ran once with seed 611. gen6b's file was fresh at 08:03 (seed 612) and appended once
  with seed 613; the two earlier starts were in deleted files.
- **Cause:** fixed-node play from the same start position replays the same game. 62–70% of the games from 22–24-piece
  random openings (4–12 random plies from the initial position) are copies (up to 117 of one game). Seeded starts (10–17
  pieces) repeat in only 1–8%. The same opening scheme was used for every older generation.
- **Fixes:**
  - datagen now uses each start position once per run;
  - its random streams depend on the records already in the output, so a same-seed restart cannot replay games;
  - it refuses partial-record files;
  - training on E5 uses deduplicated copies (`scripts/dedupe_games.py`: the first copy of each game over all files).
- **Restarts:** both generators were restarted on the fixed binary at 17:42–17:43 (sha256 df0a5f57…, branch ba45aca;
  gen6a seed 614, gen6b seed 615).
  - An old `e5_gen.sh` started by `after_gen4a.sh`, whose script file had been rewritten under it, appended to gen6a for
    about a minute with seed 611. gen6a was cut back to its last verified length (66,707,456 bytes).
- **Measured (18:15):** 9,350,912 records, 6,736,601 distinct (72.0%; 258,952 of 340,920 games).
- **Forecast (not a measurement):** about 2.0M distinct positions per hour; 15M distinct around 22:20, 30M around 06:00
  on 2026-11-11.

**Validation tails.** For flagged files they now start at a game start. Even so, many validation positions also occur
in training: 38.2% for gen6a and 63.7% for gen5a (shared openings and transpositions). Validation losses therefore
compare nets with each other but are not an independent estimate of generalisation.

**E3 dropped.** The laptop's direct match n8c vs n8bn at 0.5 s, 805 pairs: −1.7 [−5.4, +1.9] (origin/t3/search-lab
d6181de). Re-searched scores add nothing to the database-relabelled results. The rescore of the remaining old files is
dropped for good, and its partial output (gen5c, 135 KB) is deleted.

<!-- PHASE3MORE -->
"""
t = t.replace("<!-- PHASE3MORE -->\n", block, 1)
open(R, "w").write(t); print("ok")
