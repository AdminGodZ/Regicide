# Remaining work: another machine only

No command in this file was executed after the stop order. All unfinished items are **not done on this machine**.
The running query was ended immediately: R35, n6c, forced 24-28, 100M nodes, depth target 33. It has no final score.

Copy this entire round14 directory and the database to the other machine. The source snapshot master is 85a9fe8
with n6c. These commands are for a POSIX C++26 environment; use the production Windows toolchain for native Windows.
walk_remote.cpp changes only path configuration and explicit file loading of n6c; it was not compiled or tested here.

    cmake -S master -B remote-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DREGICIDE_NATIVE=OFF
    cmake --build remote-build -j 4 --target regicide_core regicide_egdb
    c++ -std=c++26 -O3 -I master/src -I booklab/tools/booklab walk_remote.cpp remote-build/libregicide_core.a remote-build/libregicide_egdb.a -pthread -o walk_remote
    export REGICIDE_REVIEW_DB=/absolute/path/to/egdb
    export REGICIDE_REVIEW_N6=/absolute/path/to/round14/master/src/weights/net.bin

n6c file SHA256 must be 6b335193073ef5f076e28b98407f75da194902705e9cf35b85f43155d29efc5e. Engine cap is seven pieces, one thread, hash 128 MiB, book/exclusions off.
The driver replays the saved record to restore the reversible counter and game history. FEN alone is insufficient.

UNFINISHED_SEARCHES.tsv gives an exact next command, FEN, net, budget, target depth and forced move for each of
the 43 unfinished games. REMAINING_POSITIONS.tsv gives all 958 remaining distinct
position contexts and root-20M/root-100M/played-100M commands. Root-100M and forced follow-ups are conditional;
the planner chooses them from completed results. It is not necessary to run every possible command.

The interrupted command is:

    ./walk_remote force wins R35 n6c 100000000 33 24-28 wins/searches/R35-n6c-100000000-force-33-24_28.tsv

To resume the saved adaptive workflow on the OTHER machine, replace its walk binary and remove the stop sentinel:

    cp walk_remote walk
    rm PAUSE_QUERIES
    python3 win_plan.py
    python3 win_batch.py

Repeat win_batch.py, always sequentially, until wins/state.json has done=true. Each batch is bounded and each
query is capped at 100M nodes. The previous queried prefix need not be repeated. Saved force-iteration traces
allow equal-depth comparisons without rerunning the same earlier iterations; derived scores carry derived_from.

Additional unfinished checks:

- Final-head execution rechecks of the three findings: not done on this machine. Build current-head core/interface
  on the other machine, run publication_stale.cpp, and rerun the two saved Python fixtures against current Book Lab.
  publication_stale.cpp needs its long net pathname adjusted to REGICIDE_REVIEW_N6; maintain a pathname longer
  than small-string storage. The exact input is successful set book 0 followed by get book, with one publication
  allocation failure; no engine node budget applies.
- The king-run repair fixture is king-fixture/bl-f1-cake-a-h1/games.pdn with base king-fixture/avoid.bin. The
  public Engine fixture is king_policy_check.cpp, root B:WK18,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,K5,13,
  reversible count 1, HCE, one thread, depth 1. No larger search is required.
- The FEN replay/report fixture is coverage-fixture/fen-win/games.pdn, the saved valid terminal White win.
  Run wins_vs_book.py with this run and current pinwalk. Correct FEN replay produces 16 own rows; dropping FEN
  produces ERR at ply 6 and must not be certified as yes. No neural net or node budget applies.
- Transposition-aware repeatability for the newly qualified #134 point and newly committed r11d records:
  not done on this machine. Compile book_line.cpp, then run its pool mode with repeat-pool-latest.tsv and
  critical-targets-closing.tsv. This is board replay, no engine search or net:

    c++ -std=c++26 -O2 -I master/src book_line.cpp remote-build/libregicide_core.a remote-build/libregicide_egdb.a -pthread -o book_line
    ./book_line pool repeat-pool-latest.tsv critical-targets-closing.tsv > repeat-hits-latest.tsv

  Adjust book_line.cpp's database path before compilation. It uses the r11c embedded in the saved master.
- Full legal-move replay of later r11d records: not done on this machine. Compile current tools/booklab/pinwalk.cpp
  and feed later-records/replay-input.tsv:

    ./pinwalk-current master/src/weights/book.bin < later-records/replay-input.tsv > later-records/replay-output.tsv

  Require no ERR lines. No search/net/node budget is needed. WDL was parsed from these records after stopping,
  but this is not a claim that their moves were replayed here.
- Measurements against the later master/its built-in n8q, or with PR #65's history-aware table-root rule:
  not done on this machine. They are a separate experiment, not interchangeable with the saved 85a9fe8/n6c
  values. Use the same position/history manifests and explicit net file with 20M and 100M node budgets.

Exact fixture command lines on the OTHER machine (source paths are the saved snapshot; use fetched current
Book Lab sources for final-head revalidation):

    c++ -std=c++26 -O2 -I master/src -I booklab-current/tools/booklab booklab-current/tools/booklab/pinwalk.cpp remote-build/libregicide_core.a remote-build/libregicide_egdb.a -pthread -o pinwalk-current
    gzip -dc booklab/experiments/booklab/wp11c/pins_r11c.txt.gz > pins_r11c.txt
    python3 booklab-current/tools/booklab/wins_vs_book.py --book master/src/weights/book.bin --pins pins_r11c.txt --decisions decisions_r11c.txt --pinwalk ./pinwalk-current --egdb "$REGICIDE_REVIEW_DB" --runs coverage-fixture/fen-win
    python3 booklab-current/tools/booklab/pins.py --runs king-fixture/bl-f1-cake-a-h1 --book king-fixture/avoid.bin --pinwalk ./pinwalk-current --egdb "$REGICIDE_REVIEW_DB" --out king-fixture/recheck-pins.txt --report king-fixture/recheck-report.md
    c++ -std=c++26 -O2 -I master/src king_policy_check.cpp remote-build/libregicide_core.a remote-build/libregicide_egdb.a -pthread -o king_policy_check
    ./king_policy_check

For current interface revalidation, build the current checkout's core and link its src/cb_api.cpp with
publication_stale.cpp; adjust that reproducer's long pathname to the absolute n6c file before compilation:

    c++ -std=c++26 -O1 -g -I "$CURRENT_REGICIDE/src" publication_stale.cpp "$CURRENT_REGICIDE/src/cb_api.cpp" "$CURRENT_BUILD/libregicide_core.a" "$CURRENT_BUILD/libregicide_egdb.a" -pthread -o publication_stale_current
    ./publication_stale_current

This is an allocation-fault test, no position or engine node budget. It has not been executed on the final fetched
master here. The original fixed-failure control is publish_exception.cpp, built the same way; update its long net
path likewise. These commands are instructions only and must not be run on the review Mac.
