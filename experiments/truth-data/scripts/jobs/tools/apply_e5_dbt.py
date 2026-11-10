import os, re
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
full = open(os.path.expanduser("~/regicide-jobs/truth/db_truth_full.md")).read().strip()
# database-truth table: replace the block that starts with "proven positions" in that section
sec = t.index("### Database-truth metric")
m = re.compile(r"proven positions \d+[^\n]*\n(\|[^\n]*\n)+").search(t, sec)
assert m and m.start() < t.index("## Phase 2"), "db table not found"
note = ("\n(Per-record evals: `data/db_truth_per_record.csv.gz`; held-out set: `data/heldout_proven_*_9_12.bin.gz`. "
        "Updated 2026-11-10 with every net trained since, including n8bn2, n8q and the wide nets.)\n")
t = t[:m.start()] + full + "\n" + note + t[m.end():]
# E5 progress: measurement and forecast apart
i0 = t.index("**Progress**")
i1 = t.index("\n\n", i0)
new = ("**Measured (2026-11-10 16:45, `data/e5_census.txt`):** 6,405,632 positions with ≥ 9 pieces in 233,290 games "
       "(gen6a 3,214,080 in 117,048 games; gen6b 3,191,552 in 116,242), game boundaries from the GAME_START flags, no "
       "record with ≤ 8 pieces. Seeds: `data/e5_seed_provenance.txt` (sha256 of the seed file, the command, the 126 PDN "
       "files).\n\n**Forecast (not a measurement):** about 1.9M positions per hour while both processes run (12 threads × "
       "158k per CPU-hour); 30M would be reached after about 12–13 more hours of running, i.e. around 2026-11-11 "
       "05:00–08:00 depending on pauses. Then n9a: n8q's recipe (incl. the ≤ 8-piece group) + this data ×3, against "
       "the best n8 net.")
t = t[:i0] + new + t[i1:]; n = 1
assert n == 1, n
open(R, "w").write(t); print("ok")
