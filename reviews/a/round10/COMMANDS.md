# Round 10 command record and reproduction notes

The audit runs under WSL user admingod, with the working directory /home/admingod/regicide-review-a.
All binary inputs come from this clone's Git objects. The shared egdb directory is read-only.

## Source inputs

- master 7f3ac697bc6dc69630b3b55bad92ad00e428aa4c
- before core fdd154e (Round 8 build of f3e8589; src and CMakeLists.txt identical)
- after core 7f3ac69 (Round 9 build of e027cf4; src and CMakeLists.txt identical)
- broad record snapshot and book: Book Lab 7ca6c27198ea0cc89f315744448fdc005ea81423
- mo3 records: Book Lab bfce8b08a4d186e9b198cc80e437ecb42e44ec35
- deck: official acf156 order from the earlier independently checked ballot export, included as ballots.tsv
- built-in n8q net SHA-256: 681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24

Source archives are under reports/round10/master-src and booklab-src. The two baseline libraries are the existing private review builds, with their source differences verified empty. No checkout or production branch was modified.

A fresh equivalent core build uses Clang 22, C++26, Release, native CPU, LTO off, no source changes:

    nice -n 19 cmake -S SOURCE -B SOURCE/build -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release -DREGICIDE_LTO=OFF -DREGICIDE_TOOLS=ON
    nice -n 19 timeout 540 cmake --build SOURCE/build -j2 --target regicide

No build command in this round needs more than two jobs. The reused build configurations above came from the earlier review rounds.

## Drivers actually compiled

For the after driver:

    nice -n 19 timeout 120 clang++ -std=c++26 -O3 -march=native -DREGICIDE_EVAL_BANDS=1 -Ireports/round10/master-src/src -Ireports/round10/master-src/tools/common reports/round10/budget.cpp reports/round10/master-src/tools/common/checkers.cpp reports/round9/claim-src/build/libregicide_core.a reports/round9/claim-src/build/libregicide_egdb.a -pthread -o reports/round10/budget-after

For the before driver, the same command uses the Round 8 include/library path and produces budget-before.
book_audit.cpp is compiled with the after core in the same way.
dip_legal.cpp uses only tools/common/checkers.cpp with -std=c++20 -O2.

The independent record referee replay_records.cpp is included here, compiled against tools/common/checkers.cpp as reports/round10/replay_records. The book audit and recount both read the included reports/round10/ballots.tsv.

## Runs

    nice -n 19 timeout 540 reports/round10/book_audit > reports/round10/evidence/book-audit.log
    nice -n 19 timeout 540 reports/round10/budget-before < reports/round10/draw-requests.tsv > reports/round10/evidence/draw-before.tsv
    nice -n 19 timeout 540 reports/round10/budget-after < reports/round10/draw-requests.tsv > reports/round10/evidence/draw-after.tsv
    nice -n 19 timeout 240 python3 reports/round10/record_check.py
    nice -n 19 python3 reports/round10/finish_tables.py

The dip requests were split into three foreground invocations, with timeout 540 each:
lines 1-3, lines 4-21, lines 22-39 of dips-requests.tsv. Each invocation used budget-after.
No searches ran concurrently. No call approached the ten-minute limit.

The record preparation script reads gzip PDNs from the own-clone archive, never live production directories. It produces private records.json / records.tsv; these large intermediates are deliberately not committed. Draw requests preserve selected histories directly, so the comparisons can be rerun without those intermediates.

Prepare the broad record IDs at 7ca6c27 first, then archive the later mo3 directories and run record_check.py. append_mo3.py appends those 936 games after the frozen IDs for the final book/reachability audit. This keeps IDs in draw-requests.tsv aligned with records.json. The new commit changes none of the candidate/decisions/dips/source-pin files.

Book graph semantics: our turns use the candidate's margin-0 book choice with its avoid list and engine eligibility. Opponent turns use every legal reply (strong check), or only observed opponent replies at that position (second check). Positions reached after a book move or opponent reply are also recorded before checking whether another book move is possible. A non-book move by Regicide terminates that path. Both Regicide colours and all 156 ballots are roots.

The book audit's final return code is zero. Logs are in evidence/. The legality table is header-only because it records only failures; there were none.
