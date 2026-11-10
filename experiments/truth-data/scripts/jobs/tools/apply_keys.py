import os
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
old_m = "- **Measured (18:15):** 9,350,912 records, 6,736,601 distinct (72.0%; 258,952 of 340,920 games)."
assert old_m in t
t = t.replace(old_m, "- **Measured (18:15):** 9,350,912 records; 6,736,601 left after removing exactly repeated games (72.0%; 258,952 of\n  340,920 games). This is the *game-deduplicated record count*, not a count of unique positions.")
old_f = "- **Forecast (not a measurement):** about 2.0M distinct positions per hour; 15M distinct around 22:20, 30M around 06:00\n  on 2026-11-11."
assert old_f in t
t = t.replace(old_f, "- **Forecast (not a measurement):** about 2.0M game-deduplicated records per hour; 15M around 22:40, 30M around\n  06:10 on 2026-11-11.")
old_v = """**Validation tails.** For flagged files they now start at a game start. Even so, many validation positions also occur
in training: 38.2% for gen6a and 63.7% for gen5a (shared openings and transpositions). Validation losses therefore
compare nets with each other but are not an independent estimate of generalisation."""
assert old_v in t
new_v = """**Diversity, measured 2026-11-10 ~19:25** (`scripts/unique_keys.py`; key = board + side to move):

| Data | Records | After removing repeated games | Unique board+side keys |
|---|---|---|---|
| E5 (gen6a + gen6b) | 10,748,672 | 8,565,918 | 7,334,907 (68.2%) |
| gen5a (old, 50k nodes) | 10,435,120 | n/a (no game flags) | 4,892,113 (46.9%) |

- Start positions can still recur across invocations: the per-run check does not see earlier runs. The
  game-deduplicated count removes only byte-identical games.
- Unique keys are the real diversity: E5 has 1.5× gen5a's.

**Validation tails.** For flagged files they now start at a game start. That does not make validation positions unique
against training. Share of validation positions whose key also occurs in the same file's training part:
- gen6a 9.1% (earlier 38.2%, before the repeated starts were fixed and the tail filled with new games);
- gen6b 14.4%;
- gen5a 63.7% (shared openings and transpositions).

Validation losses therefore compare nets with each other, but they are not an independent estimate of generalisation.
These shares are stated again next to n9h's and n9a's validation losses."""
t = t.replace(old_v, new_v)
open(R, "w").write(t); print("ok")
