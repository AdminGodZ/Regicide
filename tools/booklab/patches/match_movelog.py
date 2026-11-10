#!/usr/bin/env python3
"""Booklab variant of the match runner (master tools/match, PR #49 timing): logging only, no change of behaviour.

  match_movelog.py path/to/tools/match/main.cpp     (patches the file in place)

For every engine move the runner writes one row to games/<game>.moves.tsv next to the game's PDN:
  ply (moves before this one, ballot included) <TAB> side (B/W) <TAB> bank before the move, including this move's
  credit (CheckerBoard banking: the runner's own c.remaining) <TAB> offered maxtime <TAB> runner wall time
  <TAB> host time ("time S", -1 if none) <TAB> charged time (what the runner deducted)
T1 (experiments/time-policy/README.md) reads these instead of reconstructing the bank (review 2026-11-11).
"""
import sys

p = sys.argv[1]
s = open(p).read()


def rep(old, new):
    global s
    assert s.count(old) == 1, old[:60]
    s = s.replace(old, new)


rep("""    std::vector<std::string> comments;
    double time_used[2] = {0, 0};  // by colour""",
    """    std::vector<std::string> comments;
    std::vector<std::string> timelog;  // booklab: one row per engine move (games/<n>.moves.tsv)
    double time_used[2] = {0, 0};  // by colour""")
rep("""        std::snprintf(buf, sizeof buf, " maxtime %.4f", maxtime);""",
    """        const double bank_before = c.remaining;  // booklab move log: the runner's own bank, credit included
        std::snprintf(buf, sizeof buf, " maxtime %.4f", maxtime);""")
rep("""        g.moves_made[side]++;
""", """        g.moves_made[side]++;
        {
            char lb[200];
            const double charged = cfg.time_mode == TimeMode::CbMatch ? r.charged() : r.elapsed;
            std::snprintf(lb, sizeof lb, "%zu\\t%c\\t%.4f\\t%.4f\\t%.4f\\t%.4f\\t%.4f\\n", history.size(),
                          side == 0 ? 'B' : 'W', bank_before, maxtime, r.elapsed, r.engine_time, charged);
            g.timelog.push_back(lb);
        }
""")
rep("""        std::ofstream(fs::path(cfg.out_dir) / "games" / name) << text;""",
    """        std::ofstream(fs::path(cfg.out_dir) / "games" / name) << text;
        {
            std::ofstream ml(fs::path(cfg.out_dir) / "games" / (std::string(name) + ".moves.tsv"));
            ml << "ply\\tside\\tbank_before\\tmaxtime\\twall\\thost_time\\tcharged\\n";
            for (const auto& row : g.timelog) ml << row;
        }""")
open(p, "w").write(s)
print("patched", p)
