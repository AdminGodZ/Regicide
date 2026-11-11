# Reviewer B round3, task2 continuation

2026-11-11 15:45 UTC

Status: final at the lead request; exact continuation queues listed

The original116-game priority queue continues at rank21, and the36-entry mining queue at entry13. Complete own-game prefix now 22/116 (first20 in B2); predecessor prefix 23/116; this round has 68 completed own positions and 3 opponent predecessors. Mining prefix is 34/36, with 22 newly completed entries. New supported contexts: own 0, predecessors 0, mining boards 6. Accepted points are referenced rather than nominated again; unstable win scores, two winning moves and unrestricted-root disagreements are separated below.

The frequency pool has8880 finished games:7944 unchanged round2 inputs plus936 newly replayed mo1 games, including6976 Cake/KingsRow games and1904 GuiNN games. All53 legacy compressed PDN hashes match current booklab; all936 new main lines replay legally. The three new runs are concurrency1 and retain their run, worker and both process ages. Their312 scheduled ages0–311 and per-run results are in mo1-validation.tsv. The original116 selected draws and their ranks stay unchanged; mo1 extends the census. Later mo2/mo3 and repair-validation runs are outside this fixed56-file pool.

The continuation began on fdd154e. After PR #81 merged as 7f3ac69, the core and drivers were rebuilt; Worker search is unchanged, and the new table-draw root filter does not apply to the larger qualifying boards. The one cached table-drawn endpoint was freshly queried. A full100M ply67 verification matched every completed row. Query headers/manifest distinguish the revisions; see ../current-core/pr81-validation.txt and pr81-endpoint-refresh.txt. Queries use built-in n8q, eight-piece WLD, one thread, nice19, fresh128MiB hash, full record replay, book and avoid off. No match or opponent program is run. Stable cache contexts include FEN, reversible count and complete reversible history trail; unchanged B2 traces are copied only when needed, marked in their headers and manifest. The current-core rebuild/source audit and an exactly matching full100M trace are documented in ../current-core/. No requested context is already a completed rule draw.

Own/predecessor protocol:20M at each requested move; when its choice differs, run100M and compare the same alternative against the recorded move at equal completed depths. The deeper restricted trace supplies both reference depths; if either cannot reach the unrestricted reference, use their largest common completed depth. A numeric improvement is >30 at both, with the100M own/predecessor root still rejecting the played move. A20M agreement or sole legal move completes that protocol check without an extra100M query. Mining forces every supplied-evidence move and any additional root choice; forced-pair qualification and agreement of both unrestricted roots are separate.

Scores are from the mover’s view. Mate/database classes are compared by value class, not distance differences counted as men. A direct winning child is probed as Loss for the opponent, with immediate rule draws and terminal priority checked. Engine-known database scores require >=15000 with table hits; a shallow score that disappears at100M is flagged unstable. Search-backed scores outside direct root-table coverage remain engine opinions, not independent proofs of a full game result.

Completed trace files 201: 201 new and 0 reused. Maximum actual nodes 99,999,749; longest query 47.450s. All complete traces meet the100M actual-node and780s limits.

## End-of-day stop and exact continuation

The lead ended round 3 while mining entry 35 was running. The active forced 11-15 query finished naturally; no further query was started. The queue runner was stopped before it could continue.

- Own-game queue: ranks 1–22 complete (1–20 in B2); rank 23, R7398, ACF #16 as White against KingsRow, complete through ply 134, 47 of 88 requested positions. Resume at ply 136 in tests/v11d-n8q-kingsrow-a-h1/games.pdn.gz, round 14.2.
- Opponent predecessor queue: ranks 1–23 complete; resume at rank 24, R245, ACF #139 as Black against Cake, opponent ply 24, played 22-18, wp11c/games/bl-v11c-n6c-cake-a-h2.pdn.gz, round 47.1. The source/round fields in predecessor/game coverage are authoritative.
- Mining queue: entries 1–34 complete (1–12 in B2). Entry 35, node 441.18, ACF #162 against GuiNN, is partial: 20M root, 100M root and forced 11-15 at target depth 29 are saved. Next is forced 4-8 at 100M, target depth 29, context C0883, source game R933 at ply 19; then finish the cached pair comparisons. Entry 36 has not been queried. See partial-mining.tsv for the exact source, FEN and saved scores.

This continuation completed 68 own positions, three predecessors and 22 full mining entries. It found no qualifying missed win in the own/predecessor scope checked, and six new qualifying GuiNN mining boards. Entry 35 is pending, not a negative result. All six mining findings are finite search assessments; none is a direct root-database proof.

## Coverage and stopping point

| Rank | Game/round | Opening | Ours | Peak | Own done/requested | Own complete | Predecessor |
|---|---|---|---|---|---|---|---|
| 21 | R134/64.1 | ACF #72 | B | 98 | 12/12 | True | True |
| 22 | R6143/32.2 | ACF #36 | W | 90 | 9/9 | True | True |
| 23 | R7398/14.2 | ACF #16 Wilderness II | W | 88 | 47/88 | False | True |

Next own query: rank23, tests/v11d-n8q-kingsrow-a-h1/games.pdn.gz, round14.2, ACF #16 Wilderness II, oursW, ply136, played6-2; peak+88.

Next mining entry: rank35, node441.18, ACF #162, guinn, oursW, ply19.

Full116-game and36-entry backlogs are in game-coverage.tsv and mining-coverage.tsv. Each records completed prior-round work separately from this round.

## Position summary

| Points | Role | Opening | Ours | Opponent | FEN | Played | Games reaching | Games with move | Our score in those games | C1 games with move | Alternatives | Classification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Q001,Q002 | opponent | ACF #153 Black Widow | B | guinn | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14 | 5 | 2 | +2 =0 -0; 2.0/2 | 0 | 29-25; 32-27 | new supported improvement |
| Q003 | opponent | ACF #174 Skunk | W | guinn | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 9-14 | 7 | 2 | +2 =0 -0; 2.0/2 | 0 | 10-15 | new supported improvement |
| Q004 | opponent | ACF #25 | W | guinn | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 4-8 | 9 | 8 | +8 =0 -0; 8.0/8 | 1 | 9-13 | new supported improvement |
| Q005,Q006 | opponent | ACF #34 Twilight Zone | W | guinn | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 9-14 | 5 | 4 | +4 =0 -0; 4.0/4 | 1 | 15-19; 7-11 | new supported improvement |
| Q007,Q008,Q009 | opponent | ACF #71 Diabolical Denny | W | guinn | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 6 | 3 | +3 =0 -0; 3.0/3 | 0 | 14-18; 2-7; 3-7 | new supported improvement |
| Q010 | opponent | ACF #10 | W | guinn | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 4-8 | 4 | 2 | +2 =0 -0; 2.0/2 | 0 | 1-6 | new supported improvement |

## Qualifying comparisons

| Point | Kind | Rank | Node | Opening | Ours | Opponent | Ply | FEN | Played | Alternative | 20M pair | 100M pair | Stable DB score | Both win | Roots reject played | Classification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Q001 | mining | 13 | 422.17 | ACF #153 Black Widow | B | guinn | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14 | 29-25 | -215 / -9 at d24 (gain 206) | -205 / -15 at d30 (gain 190) | False | False | True | new supported improvement |
| Q002 | mining | 13 | 422.17 | ACF #153 Black Widow | B | guinn | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14 | 32-27 | -215 / -51 at d24 (gain 164) | -205 / -40 at d30 (gain 165) | False | False | True | new supported improvement |
| Q003 | mining | 16 | 463.10 | ACF #174 Skunk | W | guinn | 11 | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 9-14 | 10-15 | -193 / -29 at d24 (gain 164) | -197 / -16 at d27 (gain 181) | False | False | True | new supported improvement |
| Q004 | mining | 19 | 481.14 | ACF #25 | W | guinn | 15 | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 4-8 | 9-13 | -94 / 0 at d27 (gain 94) | -120 / 0 at d30 (gain 120) | False | False | True | new supported improvement |
| Q005 | mining | 20 | 497.12 | ACF #34 Twilight Zone | W | guinn | 13 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 9-14 | 15-19 | -145 / -98 at d23 (gain 47) | -186 / -21 at d27 (gain 165) | False | False | True | new supported improvement |
| Q006 | mining | 20 | 497.12 | ACF #34 Twilight Zone | W | guinn | 13 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 9-14 | 7-11 | -145 / -42 at d23 (gain 103) | -186 / -105 at d27 (gain 81) | False | False | True | new supported improvement |
| Q007 | mining | 27 | 569.8 | ACF #71 Diabolical Denny | W | guinn | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 14-18 | -74 / -31 at d23 (gain 43) | -67 / -36 at d27 (gain 31) | False | False | True | new supported improvement |
| Q008 | mining | 27 | 569.8 | ACF #71 Diabolical Denny | W | guinn | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 2-7 | -74 / -25 at d23 (gain 49) | -67 / -36 at d27 (gain 31) | False | False | True | new supported improvement |
| Q009 | mining | 27 | 569.8 | ACF #71 Diabolical Denny | W | guinn | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 3-7 | -74 / -20 at d23 (gain 54) | -67 / -32 at d27 (gain 35) | False | False | True | new supported improvement |
| Q010 | mining | 33 | 315.16 | ACF #10 | W | guinn | 17 | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 4-8 | 1-6 | -145 / 0 at d26 (gain 145) | -170 / 0 at d30 (gain 170) | False | False | True | new supported improvement |

## Frequencies at qualifying boards

Full FEN matching counts move-order transpositions across openings. Own points use Cake/KingsRow games where Regicide moves; opponent points use the named opponent. Counts are distinct games and visits separately, with outcomes from Regicide’s side. A repeated board can contribute to multiple move columns in the same game. Board equality does not imply identical reversible histories. C1 columns identify the new mo1 subset; qualifying-occurrences.tsv retains every matching game, run and move.

| Point | FEN | All games | Visits | Move | Games with move | Visits with move | Our W | D | L | C1 games | C1 visits | Our points | Possible points |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Q001 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 17-14 | 2 | 2 | 2 | 0 | 0 | 0 | 0 | 2.0 | 2 |
| Q001 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 29-25 | 2 | 2 | 0 | 2 | 0 | 0 | 0 | 1.0 | 2 |
| Q001 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 32-27 | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 1.0 | 1 |
| Q002 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 17-14 | 2 | 2 | 2 | 0 | 0 | 0 | 0 | 2.0 | 2 |
| Q002 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 29-25 | 2 | 2 | 0 | 2 | 0 | 0 | 0 | 1.0 | 2 |
| Q002 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 5 | 5 | 32-27 | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 1.0 | 1 |
| Q003 | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 7 | 7 | 10-15 | 5 | 5 | 2 | 3 | 0 | 1 | 1 | 3.5 | 5 |
| Q003 | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 7 | 7 | 9-14 | 2 | 2 | 2 | 0 | 0 | 0 | 0 | 2.0 | 2 |
| Q004 | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 9 | 9 | 4-8 | 8 | 8 | 8 | 0 | 0 | 1 | 1 | 8.0 | 8 |
| Q004 | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 9 | 9 | 9-13 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0.5 | 1 |
| Q005 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 5 | 5 | 15-19 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.0 | 0 |
| Q005 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 5 | 5 | 7-11 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0.5 | 1 |
| Q005 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 5 | 5 | 9-14 | 4 | 4 | 4 | 0 | 0 | 1 | 1 | 4.0 | 4 |
| Q006 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 5 | 5 | 7-11 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0.5 | 1 |
| Q006 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 5 | 5 | 9-14 | 4 | 4 | 4 | 0 | 0 | 1 | 1 | 4.0 | 4 |
| Q007 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 12-16 | 3 | 3 | 3 | 0 | 0 | 0 | 0 | 3.0 | 3 |
| Q007 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 14-18 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.0 | 0 |
| Q007 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 3-7 | 3 | 3 | 0 | 3 | 0 | 1 | 1 | 1.5 | 3 |
| Q008 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 12-16 | 3 | 3 | 3 | 0 | 0 | 0 | 0 | 3.0 | 3 |
| Q008 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 2-7 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0.0 | 0 |
| Q008 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 3-7 | 3 | 3 | 0 | 3 | 0 | 1 | 1 | 1.5 | 3 |
| Q009 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 12-16 | 3 | 3 | 3 | 0 | 0 | 0 | 0 | 3.0 | 3 |
| Q009 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 6 | 6 | 3-7 | 3 | 3 | 0 | 3 | 0 | 1 | 1 | 1.5 | 3 |
| Q010 | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 4 | 4 | 1-6 | 2 | 2 | 0 | 2 | 0 | 0 | 0 | 1.0 | 2 |
| Q010 | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 4 | 4 | 4-8 | 2 | 2 | 2 | 0 | 0 | 0 | 0 | 2.0 | 2 |

## All completed own and predecessor checks

### Own moves

| Rank | Game/ply | Played | 20M root | 100M root | 20M pair | 100M pair | Status |
|---|---|---|---|---|---|---|---|
| 21 | R134/37 | 10x19 | 10x19 / 95 / d1 | - | - | - | only legal move |
| 21 | R134/39 | 8-12 | 8-12 / 0 / d32 | - | - | - | 20M chooses played move |
| 21 | R134/41 | 19-24 | 19-24 / 0 / d37 | - | - | - | 20M chooses played move |
| 21 | R134/43 | 13-17 | 13-17 / 0 / d36 | - | - | - | 20M chooses played move |
| 21 | R134/45 | 14-18 | 14-18 / 0 / d37 | - | - | - | 20M chooses played move |
| 21 | R134/47 | 16x23x30 | 16x23x30 / 0 / d36 | - | - | - | 20M chooses played move |
| 21 | R134/49 | 30-26 | 30-26 / 0 / d37 | - | - | - | 20M chooses played move |
| 21 | R134/51 | 12-16 | 12-16 / 0 / d37 | - | - | - | 20M chooses played move |
| 21 | R134/53 | 26-31 | 26-31 / 0 / d35 | - | - | - | 20M chooses played move |
| 21 | R134/55 | 31-26 | 31-26 / 0 / d38 | - | - | - | 20M chooses played move |
| 21 | R134/57 | 20-24 | 20-24 / 0 / d63 | - | - | - | 20M chooses played move |
| 21 | R134/59 | 26x19 | 26x19 / 0 / d1 | - | - | - | only legal move |
| 22 | R6143/30 | 20x11 | 20x11 / 51 / d1 | - | - | - | only legal move |
| 22 | R6143/32 | 26-22 | 26-22 / 0 / d32 | - | - | - | 20M chooses played move |
| 22 | R6143/34 | 18x9 | 18x9 / 59 / d1 | - | - | - | only legal move |
| 22 | R6143/36 | 22-17 | 22-18 / 0 / d35 | 22-18 / 0 / d41 | 0 / 0 at d35 (gain 0) | 0 / 0 at d41 (gain 0) | does not qualify at both references |
| 22 | R6143/38 | 23x14 | 23x14 / 90 / d1 | - | - | - | only legal move |
| 22 | R6143/40 | 14-9 | 14-9 / 0 / d45 | - | - | - | 20M chooses played move |
| 22 | R6143/42 | 9-5 | 9-5 / 0 / d45 | - | - | - | 20M chooses played move |
| 22 | R6143/44 | 13x6 | 13x6 / 0 / d1 | - | - | - | only legal move |
| 22 | R6143/46 | 17-13 | 17-13 / 0 / d127 | - | - | - | 20M chooses played move |
| 23 | R7398/42 | 15-11 | 15-11 / 31 / d31 | - | - | - | 20M chooses played move |
| 23 | R7398/44 | 11-7 | 6-1 / 30 / d30 | 6-1 / 29 / d36 | 37 / 29 at d30 (gain -8) | 32 / 18 at d36 (gain -14) | does not qualify at both references |
| 23 | R7398/46 | 6-1 | 6-1 / 20 / d32 | - | - | - | 20M chooses played move |
| 23 | R7398/48 | 1-6 | 1-6 / 25 / d32 | - | - | - | 20M chooses played move |
| 23 | R7398/50 | 6-10 | 6-10 / 30 / d30 | - | - | - | 20M chooses played move |
| 23 | R7398/52 | 7-2 | 7-2 / 33 / d30 | - | - | - | 20M chooses played move |
| 23 | R7398/54 | 10-15 | 10-15 / 32 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/56 | 31-26 | 31-26 / 19 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/58 | 2-6 | 2-6 / 24 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/60 | 15-18 | 15-18 / 44 / d30 | - | - | - | 20M chooses played move |
| 23 | R7398/62 | 18-14 | 18-14 / 74 / d32 | - | - | - | 20M chooses played move |
| 23 | R7398/64 | 26-22 | 26-22 / 69 / d33 | - | - | - | 20M chooses played move |
| 23 | R7398/66 | 22-17 | 6-10 / 46 / d32 | 6-10 / 39 / d38 | 48 / 55 at d32 (gain 7) | 33 / 39 at d36 (gain 6) | does not qualify at both references |
| 23 | R7398/68 | 6-10 | 6-10 / 58 / d31 | - | - | - | 20M chooses played move |
| 23 | R7398/70 | 17-13 | 17-13 / 43 / d32 | - | - | - | 20M chooses played move |
| 23 | R7398/72 | 30-25 | 30-26 / 43 / d34 | 30-26 / 29 / d39 | 38 / 31 at d34 (gain -7) | 33 / 31 at d38 (gain -2) | does not qualify at both references |
| 23 | R7398/74 | 25-22 | 25-22 / 28 / d34 | - | - | - | 20M chooses played move |
| 23 | R7398/76 | 22-18 | 22-18 / 28 / d35 | - | - | - | 20M chooses played move |
| 23 | R7398/78 | 13-9 | 13-9 / 32 / d35 | - | - | - | 20M chooses played move |
| 23 | R7398/80 | 9-6 | 9-6 / 28 / d34 | - | - | - | 20M chooses played move |
| 23 | R7398/82 | 6-1 | 6-1 / 29 / d33 | - | - | - | 20M chooses played move |
| 23 | R7398/84 | 1-6 | 1-6 / 28 / d31 | - | - | - | 20M chooses played move |
| 23 | R7398/86 | 6-9 | 6-9 / 28 / d31 | - | - | - | 20M chooses played move |
| 23 | R7398/88 | 9-13 | 9-13 / 28 / d31 | - | - | - | 20M chooses played move |
| 23 | R7398/90 | 14-17 | 14-17 / 26 / d30 | - | - | - | 20M chooses played move |
| 23 | R7398/92 | 18-14 | 18-14 / 24 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/94 | 17-22 | 17-22 / 25 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/96 | 13-17 | 13-17 / 25 / d28 | - | - | - | 20M chooses played move |
| 23 | R7398/98 | 17-13 | 17-13 / 21 / d28 | - | - | - | 20M chooses played move |
| 23 | R7398/100 | 10-7 | 10-7 / 23 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/102 | 22-17 | 7-11 / 23 / d26 | 7-11 / 18 / d30 | 25 / 24 at d26 (gain -1) | 19 / 22 at d30 (gain 3) | does not qualify at both references |
| 23 | R7398/104 | 17-21 | 7-11 / 21 / d27 | 7-11 / 18 / d31 | 23 / 19 at d27 (gain -4) | 19 / 17 at d30 (gain -2) | does not qualify at both references |
| 23 | R7398/106 | 7-10 | 7-11 / 23 / d25 | 7-11 / 17 / d32 | 21 / 22 at d25 (gain 1) | 17 / 18 at d30 (gain 1) | does not qualify at both references |
| 23 | R7398/108 | 21-25 | 21-25 / 21 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/110 | 10-15 | 10-15 / 27 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/112 | 14-10 | 14-10 / 26 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/114 | 13-17 | 13-17 / 25 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/116 | 25-21 | 25-21 / 24 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/118 | 10-7 | 17-14 / 27 / d29 | 17-14 / 22 / d34 | 22 / 26 at d29 (gain 4) | 22 / 25 at d33 (gain 3) | does not qualify at both references |
| 23 | R7398/120 | 15-10 | 15-10 / 25 / d29 | - | - | - | 20M chooses played move |
| 23 | R7398/122 | 7-3 | 7-3 / 23 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/124 | 17-14 | 17-14 / 22 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/126 | 3-7 | 3-7 / 22 / d28 | - | - | - | 20M chooses played move |
| 23 | R7398/128 | 21-25 | 21-25 / 23 / d27 | - | - | - | 20M chooses played move |
| 23 | R7398/130 | 10-6 | 7-2 / 21 / d27 | 7-2 / 17 / d31 | 24 / 18 at d27 (gain -6) | 22 / 16 at d31 (gain -6) | does not qualify at both references |
| 23 | R7398/132 | 7-10 | 7-10 / 18 / d26 | - | - | - | 20M chooses played move |
| 23 | R7398/134 | 25-30 | 14-17 / 19 / d26 | 25-30 / 16 / d33 | 16 / 12 at d26 (gain -4) | 16 / 9 at d32 (gain -7) | does not qualify at both references |

### Opponent immediately before first+40

| Rank | Game/ply | Played | 20M root | 100M root | 20M pair | 100M pair | Status |
|---|---|---|---|---|---|---|---|
| 21 | R134/36 | 6-2 | 15-11 / 0 / d29 | 15-11 / 0 / d34 | 0 / 0 at d29 (gain 0) | 0 / 0 at d33 (gain 0) | does not qualify at both references |
| 22 | R6143/29 | 11-16 | 11-16 / 0 / d32 | - | - | - | 20M chooses played move |
| 23 | R7398/41 | 32-28 | 32-28 / -41 / d32 | - | - | - | 20M chooses played move |

All negative checks retain FEN, record and colour in own-results.tsv/predecessor-results.tsv. A dash means that query/comparison was not part of the protocol for that point.

## Every completed predecessor census

| Rank | Game | Opening | Ours | Opponent | Ply | FEN | Played | Games reaching | Visits | Games playing it | Visits playing it | All moves and our results |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 21 | R134 | ACF #72 | B | cake | 36 | W:W6,15,21,22,23,26,27,28:B3,5,8,10,13,14,16,20 | 6-2 | 5 | 5 | 5 | 5 | 6-2: 5 games/5 visits, +0 =5 -0, C1 0 |
| 22 | R6143 | ACF #36 | W | kingsrow | 29 | B:W13,18,19,20,23,26,30,32:B2,5,6,7,10,11,12,21 | 11-16 | 2 | 2 | 2 | 2 | 11-16: 2 games/2 visits, +0 =2 -0, C1 0 |
| 23 | R7398 | ACF #16 Wilderness II | W | kingsrow | 41 | B:W6,15,20,30,31:B4,5,9,12,K32 | 32-28 | 11 | 11 | 8 | 8 | 32-28: 8 games/8 visits, +0 =8 -0, C1 1; 9-14: 3 games/3 visits, +3 =0 -0, C1 0 |

## Mining continuation

| Rank | Node | Opening | Opponent | Ours | Ply | FEN | Losing-game move | Alternative | Observed draw move | 20M root | 100M root | 20M pair | 100M pair | Qualified | Roots reject played | Stable DB score | Unstable20M | Both win | Accepted reference |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 13 | 422.17 | ACF #153 Black Widow | guinn | B | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14 | 29-25 | True | 29-25 / -13 / d24 | 29-25 / -10 / d30 | -215 / -9 at d24 (gain 206) | -205 / -15 at d30 (gain 190) | True | True | False | False | False |  |
| 13 | 422.17 | ACF #153 Black Widow | guinn | B | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14 | 32-27 | False | 29-25 / -13 / d24 | 29-25 / -10 / d30 | -215 / -51 at d24 (gain 164) | -205 / -40 at d30 (gain 165) | True | True | False | False | False |  |
| 13 | 422.17 | ACF #153 Black Widow | guinn | B | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 32-27 | 17-14 | False | 29-25 / -13 / d24 | 29-25 / -10 / d30 | -51 / -215 at d24 (gain -164) | -40 / -205 at d30 (gain -165) | False | True | False | False | False |  |
| 13 | 422.17 | ACF #153 Black Widow | guinn | B | 18 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 32-27 | 29-25 | True | 29-25 / -13 / d24 | 29-25 / -10 / d30 | -51 / -9 at d24 (gain 42) | -40 / -15 at d30 (gain 25) | False | True | False | False | False |  |
| 14 | 435.8 | ACF #16 Wilderness II | guinn | W | 9 | B:W14,18,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,12,13,16 | 1-6 | 7-11 | True | 7-11 / -54 / d23 | 1-6 / -53 / d28 | -46 / -27 at d23 (gain 19) | -29 / -84 at d28 (gain -55) | False | False | False | False | False |  |
| 15 | 441.12 | ACF #162 | guinn | W | 13 | B:W9,17,21,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,10,20 | 6x13x22 | 5x14 | True | 6x13x22 / -11 / d24 | 6x13x22 / 0 / d29 | -7 / -6 at d24 (gain 1) | 0 / 0 at d28 (gain 0) | False | False | False | False | False |  |
| 16 | 463.10 | ACF #174 Skunk | guinn | W | 11 | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 9-14 | 10-15 | True | 10-15 / -41 / d24 | 10-15 / -6 / d30 | -193 / -29 at d24 (gain 164) | -197 / -16 at d27 (gain 181) | True | True | False | False | False |  |
| 17 | 477.4 | ACF #23 | guinn | W | 5 | B:W19,21,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,6,7,8,9,10,11,12,13 | 10-14 | 11-15 | True | 11-15 / -13 / d26 | 11-15 / -28 / d31 | -11 / -8 at d26 (gain 3) | -10 / -7 at d31 (gain 3) | False | True | False | False | False |  |
| 18 | 480.3 | ACF #25 | guinn | B | 4 | W:W19,21,22,23,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,11,12,13,14 | 22-18 | 27-24 | True | 22-18 / 20 / d25 | 22-18 / 18 / d29 | 16 / 15 at d25 (gain -1) | 6 / 0 at d29 (gain -6) | False | False | False | False | False |  |
| 19 | 481.14 | ACF #25 | guinn | W | 15 | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 4-8 | 9-13 | True | 9-13 / 0 / d27 | 9-13 / 0 / d31 | -94 / 0 at d27 (gain 94) | -120 / 0 at d30 (gain 120) | True | True | False | False | False |  |
| 20 | 497.12 | ACF #34 Twilight Zone | guinn | W | 13 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 9-14 | 15-19 | False | 15-19 / -37 / d23 | 7-11 / -39 / d27 | -145 / -98 at d23 (gain 47) | -186 / -21 at d27 (gain 165) | True | True | False | False | False |  |
| 20 | 497.12 | ACF #34 Twilight Zone | guinn | W | 13 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 9-14 | 7-11 | True | 15-19 / -37 / d23 | 7-11 / -39 / d27 | -145 / -42 at d23 (gain 103) | -186 / -105 at d27 (gain 81) | True | True | False | False | False |  |
| 21 | 525.10 | ACF #50 | guinn | W | 11 | B:W17,19,24,25,27,28,29,30,31,32:B1,2,3,4,5,6,8,10,12,18 | 2-7 | 8-11 | True | 8-11 / -6 / d23 | 8-11 / 0 / d29 | -17 / 0 at d23 (gain 17) | -11 / 0 at d29 (gain 11) | False | True | False | False | False |  |
| 22 | 543.8 | ACF #6 Inferno | guinn | W | 9 | B:W18,21,23,24,25,27,28,29,30,31,32:B1,2,3,4,7,8,9,11,12,13,14 | 11-16 | 7-10 | True | 7-10 / -45 / d25 | 7-10 / -45 / d29 | -86 / -32 at d25 (gain 54) | -54 / -33 at d29 (gain 21) | False | True | False | False | False |  |
| 23 | 545.18 | ACF #60 Black Hole | guinn | W | 19 | B:W14,21,22,27,29,30,31,32:B2,3,4,5,7,11,12,13 | 11-15 | 4-8 | True | 4-8 / -10 / d23 | 4-8 / -7 / d29 | -24 / -13 at d23 (gain 11) | -22 / -6 at d29 (gain 16) | False | True | False | False | False |  |
| 24 | 549.10 | ACF #62 Gemini I | guinn | W | 11 | B:W18,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,8,9,10,12,16 | 1-6 | 3-7 | True | 3-7 / -47 / d23 | 1-6 / -65 / d27 | -68 / -43 at d23 (gain 25) | -38 / -39 at d27 (gain -1) | False | False | False | False | False |  |
| 25 | 553.4 | ACF #64 | guinn | W | 5 | B:W18,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,10,11,12,14 | 1-6 | 11-15 | True | 11-15 / -43 / d26 | 11-15 / -23 / d30 | -29 / -19 at d26 (gain 10) | -18 / -18 at d30 (gain 0) | False | True | False | False | False |  |
| 26 | 557.8 | ACF #66 | guinn | W | 9 | B:W19,21,23,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,9,12,14 | 4-8 | 7-11 | True | 4-8 / -10 / d24 | 4-8 / 0 / d29 | 0 / -2 at d24 (gain -2) | 0 / 0 at d29 (gain 0) | False | False | False | False | False |  |
| 27 | 569.8 | ACF #71 Diabolical Denny | guinn | W | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 14-18 | False | 2-7 / -32 / d23 | 14-18 / -25 / d27 | -74 / -31 at d23 (gain 43) | -67 / -36 at d27 (gain 31) | True | True | False | False | False |  |
| 27 | 569.8 | ACF #71 Diabolical Denny | guinn | W | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 2-7 | False | 2-7 / -32 / d23 | 14-18 / -25 / d27 | -74 / -25 at d23 (gain 49) | -67 / -36 at d27 (gain 31) | True | True | False | False | False |  |
| 27 | 569.8 | ACF #71 Diabolical Denny | guinn | W | 9 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16 | 3-7 | True | 2-7 / -32 / d23 | 14-18 / -25 / d27 | -74 / -20 at d23 (gain 54) | -67 / -32 at d27 (gain 35) | True | True | False | False | False |  |
| 28 | 581.4 | ACF #78 | guinn | W | 5 | B:W19,21,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,8,9,12,14,16 | 7-10 | 6-10 | True | 7-10 / -15 / d26 | 7-10 / -17 / d31 | -13 / -32 at d26 (gain -19) | 0 / -21 at d30 (gain -21) | False | False | False | False | False |  |
| 29 | 601.10 | ACF #87 Tyne | guinn | W | 11 | B:W14,19,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,11,12,13,18 | 4-8 | 6-10 | True | 4-8 / -29 / d24 | 4-8 / -38 / d29 | -19 / -20 at d24 (gain -1) | -34 / -87 at d28 (gain -53) | False | False | False | False | False |  |
| 30 | 605.6 | ACF #9 Wilderness I | guinn | W | 7 | B:W14,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,12,13,16 | 6-9 | 16-20 | True | 16-20 / -62 / d24 | 7-11 / -74 / d27 | -93 / -56 at d24 (gain 37) | -59 / -56 at d27 (gain 3) | False | True | False | False | False |  |
| 30 | 605.6 | ACF #9 Wilderness I | guinn | W | 7 | B:W14,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,12,13,16 | 6-9 | 7-11 | False | 16-20 / -62 / d24 | 7-11 / -74 / d27 | -93 / -105 at d24 (gain -12) | -59 / -91 at d27 (gain -32) | False | True | False | False | False |  |
| 31 | 610.3 | ACF #92 | guinn | B | 4 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,15 | 17-14 | 24-20 | False | 17-14 / 6 / d24 | 17-14 / 0 / d30 | 0 / -2 at d24 (gain -2) | 0 / 0 at d30 (gain 0) | False | False | False | False | False |  |
| 31 | 610.3 | ACF #92 | guinn | B | 4 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,15 | 17-14 | 25-22 | True | 17-14 / 6 / d24 | 17-14 / 0 / d30 | 0 / 0 at d24 (gain 0) | 0 / 0 at d30 (gain 0) | False | False | False | False | False |  |
| 31 | 610.3 | ACF #92 | guinn | B | 4 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,15 | 24-20 | 17-14 | False | 17-14 / 6 / d24 | 17-14 / 0 / d30 | -2 / 0 at d24 (gain 2) | 0 / 0 at d30 (gain 0) | False | True | False | False | False |  |
| 31 | 610.3 | ACF #92 | guinn | B | 4 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,15 | 24-20 | 25-22 | True | 17-14 / 6 / d24 | 17-14 / 0 / d30 | -2 / 0 at d24 (gain 2) | 0 / 0 at d30 (gain 0) | False | True | False | False | False |  |
| 32 | 617.14 | ACF #96 Skull-Cracker | guinn | W | 15 | B:W14,21,25,26,27,28,29,31,32:B1,2,3,4,7,8,9,12,18 | 1-5 | 9-13 | True | 9-13 / -28 / d25 | 9-13 / -33 / d30 | -57 / -41 at d25 (gain 16) | -46 / -28 at d30 (gain 18) | False | True | False | False | False |  |
| 33 | 315.16 | ACF #10 | guinn | W | 17 | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 4-8 | 1-6 | True | 1-6 / 0 / d26 | 1-6 / 0 / d30 | -145 / 0 at d26 (gain 145) | -170 / 0 at d30 (gain 170) | True | True | False | False | False |  |
| 34 | 385.16 | ACF #134 | guinn | W | 17 | B:W13,18,23,24,27,28,29,30,31,32:B1,2,3,6,8,10,11,12,14,20 | 14-17 | 1-5 | True | 14-17 / -17 / d25 | 14-17 / -21 / d29 | -17 / 0 at d25 (gain 17) | -20 / 0 at d29 (gain 20) | False | False | False | False | False |  |

The result contrast in the supplied list crosses books/nets. In ACF34 the20M and100M unrestricted choices differ; both reject9-14, and both forced alternatives exceed the relative30 threshold. Their absolute scores vary strongly with search restriction and depth, so this does not certify a drawing alternative. Updated8880-game frequencies for every completed entry are in mining-census.tsv; they remain descriptive and are distinct from the input priority counts. Input moves are not silently expanded from new census results.

The GuiNN ACF71 ply9 result passes narrowly: the observed3-7 alternative gains54 at the20M depth reference and35 at the100M reference; the two extra root alternatives gain31 at the deeper reference. These finite scores meet the requested strict >30 threshold and do not solve the position.

## What ran and what was reused

Ran: source-byte checks, new936-game legal replay, all newly completed queries in the manifest, completed-depth/node validation and all-context history audit. Reused: unchanged7944-game legal replay, the original116-game selection and cutoff plans, and explicitly marked identical-history B2 search traces. The earlier112 Draw endpoint probes belong to B2 and are available in ../../round2/database-endpoints.tsv. No engine or book source change, database modification, training-data write, game or new broad test suite is performed.

## Reproduction sources

### prepare.py

```python
import csv,gzip,json,hashlib,subprocess,sys,shutil,collections
from pathlib import Path
P=Path('work/round3/task2');D=Path('reports/round3/task2');old=Path('work/round2')
sys.path.insert(0,'snapshots/booklab/tools/booklab')
from theory import parse_pdn,ballot_of,family
for name in ('contexts.json','jobs.json','selected.json','walk-plan.json','state.json','mining-candidates-001.tsv','mining-evidence.tsv'):
 if not (P/name).exists():shutil.copyfile(old/name,P/name)
games=json.load(open(old/'games.json'));assert len(games)==7944
legacy=list(csv.DictReader(open('reports/round2/source-files.tsv'),delimiter='\t'))
inventory=[]
for r in legacy:
 blob=subprocess.check_output(['git','show','origin/t3/booklab:experiments/booklab/'+r['path']])
 assert hashlib.sha256(blob).hexdigest()==r['sha256_pdn_gz'],r['path']
 inventory.append([r['path'],r['games'],r['sha256_pdn_gz'],'round2 pool','original run'])
for g in games:g['cohort']='round2';g['concurrency']='original run'
known={(g['run'].removeprefix('bl-'),g['rnd'],g['ours']) for g in games};new=[];checks=[]
expected={'cake':(0,312,0),'guinn':(21,291,0),'kingsrow':(1,310,1)}
for opp in ('cake','guinn','kingsrow'):
 run='mo1-'+opp;source=f'tests/{run}/games.pdn.gz';path='experiments/booklab/'+source
 blob=subprocess.check_output(['git','show','origin/t3/booklab:'+path])
 assert blob==subprocess.check_output(['git','show','e2c16559a72eccd17488d2f23c930f5b919a42bf:'+path])
 records=list(parse_pdn(gzip.decompress(blob).decode()));assert len(records)==312
 out=collections.Counter()
 for t,mc in records:
  rb=t.get('Black','').lower().startswith('regicide');rw=t.get('White','').lower().startswith('regicide');assert rb!=rw
  ours='B' if rb else 'W';o=family(t.get('White' if rb else 'Black',''));assert o==opp
  result=t.get('Result');assert result in ('1-0','0-1','1/2-1/2')
  pair,leg=map(int,t['Round'].split('.'));age=(pair-1)*2+leg-1
  assert int(t['Worker'])==0 and int(t['EngineGamesA'])==age and int(t['EngineGamesB'])==age
  key=(run,t['Round'],ours);assert key not in known,key;known.add(key)
  ballot=ballot_of(t.get('Opening',''))
  g=dict(id=f'R{len(games)}',source=source,run=run,rnd=t['Round'],fen=t.get('FEN',''),ours=ours,opp=opp,opening=ballot[0],nopen=ballot[2],result=result,moves=[m for m,c in mc],comments=[c for m,c in mc],tags=t,cohort='mo1',concurrency=1)
  games.append(g);new.append(g)
  out['D' if result=='1/2-1/2' else 'W' if result==('1-0' if ours=='B' else '0-1') else 'L']+=1
 assert (out['W'],out['D'],out['L'])==expected[opp],out
 sha=hashlib.sha256(blob).hexdigest();inventory.append([source,312,sha,'mo1',1]);checks.append([run,312,out['W'],out['D'],out['L'],'worker0; both ages equal scheduled game index'])
assert len(games)==8880 and len(new)==936
P.joinpath('games.json').write_text(json.dumps(games))
for name,gg in [('games.tsv',games),('new-games.tsv',new)]:
 with (P/name).open('w') as f:
  for g in gg:f.write('\t'.join([g['id'],g['fen'],g['ours'],str(g['nopen']),' '.join(g['moves'])])+'\n')
with (D/'source-files.tsv').open('w') as f:
 w=csv.writer(f,delimiter='\t');w.writerow(['path','games','sha256_pdn_gz','cohort','concurrency']);w.writerows(inventory)
with (D/'mo1-validation.tsv').open('w') as f:
 w=csv.writer(f,delimiter='\t');w.writerow(['run','games','our_wins','draws','our_losses','counter_check']);w.writerows(checks)
for name in ('mining-candidates-001.tsv','mining-evidence.tsv'):
 remote=subprocess.check_output(['git','show','origin/t3/review-reports:reviews/a/round3/tables/'+name])
 assert remote==(P/name).read_bytes(),name
print('Validated unchanged53 legacy PDNs; appended936 concurrency1 games; corpus8880; original116 ranked draws unchanged.')
print('C1 source checks:',checks)
print('Selected next game',json.load(open(P/'selected.json'))[20])
```

### census.py

```python
import csv,json,collections
from pathlib import Path
P=Path('work/round3/task2');D=Path('reports/round3/task2')
gs={g['id']:g for g in json.load(open(P/'games.json'))};contexts=json.load(open(P/'contexts.json'));fens={c['fen'] for c in contexts}
occ=json.load(open('work/round2/occurrences.json'))
def annotate(x):
 g=gs[x['game']];x.update(cohort=g['cohort'],concurrency=g['concurrency'],worker=g['tags'].get('Worker',''),age_a=g['tags'].get('EngineGamesA',''),age_b=g['tags'].get('EngineGamesB',''))
for x in occ:annotate(x)
seen=set();positions=0
for r in csv.DictReader(open(P/'new-walk.tsv'),delimiter='\t'):
 seen.add(r['game']);positions+=1
 if r['fen'] not in fens:continue
 g=gs[r['game']]
 x=dict(game=g['id'],run=g['run'],round=g['rnd'],source=g['source'],opening=g['opening'],ours=g['ours'],opponent=g['opp'],result=g['result'],ply=r['ply'],fen=r['fen'],rev=r['rev'],move=r['played'],role='own' if r['fen'][0]==g['ours'] else 'opponent')
 annotate(x);occ.append(x)
assert len(seen)==936
P.joinpath('occurrences.json').write_text(json.dumps(occ))
(D/'input-validation.txt').write_text(f'Legacy 7944 games: reused legal replay and 53 compressed-PDN SHA-256 hashes, all checked unchanged against current origin/t3/booklab.\nNew 936 mo1 games: all main lines legally replayed with current fdd154e core, {positions} pre-move positions; no illegal or ambiguous capture.\nAll source results finished; run/round/colour identities unique; every new worker is0 and both process ages equal scheduled game index0..311.\nFrequency corpus8880 games; original116 selected-game rank order and36 mining-entry order retained. New mo1 records are census additions.\n')
print('New legal replay936games',positions,'positions; target visits',len(occ),'includingC1',sum(x['cohort']=='mo1' for x in occ))
```

### replay.cpp

```cpp
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "search.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
#include "movetext.hpp"
using namespace regicide;
std::vector<std::string> split(const std::string&s,char c='\t'){std::vector<std::string>v;std::stringstream ss(s);for(std::string x;std::getline(ss,x,c);)v.push_back(x);return v;}
struct Game{std::string id,fen,col;int nopen;std::vector<std::string>mv;};
bool nowdraw(const Position&p,const Move&m,const std::vector<Position>&hist){Position c=p;c.make_move(m);MoveList ml;generate_moves(c,ml);if(ml.empty())return false;int rev=c.reversible_plies();if(rev>=NO_PROGRESS_PLIES)return true;int n=std::min<int>(hist.size(),std::max(rev-1,0)),count=0;for(int i=int(hist.size())-n;i<int(hist.size());++i)count+=hist[i].key()==c.key();return count>=2;}
int main(int ac,char**av){try{
if(ac<3)return 2;std::ifstream in(av[2]);std::map<std::string,Game>gs;
for(std::string l;std::getline(in,l);){auto f=split(l);Game g{f[0],f[1],f[2],std::stoi(f[3]),{}};std::istringstream ms(f[4]);for(std::string m;ms>>m;)g.mv.push_back(m);gs[g.id]=g;}
std::string mode=av[1];
if(mode=="walk"){
 std::cout<<"game\tply\tfen\trev\tplayed\tlegal\tpieces\n";
 for(auto&[id,g]:gs){Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");int ply=0;for(auto&t:g.mv){Move m;if(!parse_move(p,t,m)||booklab::matching_moves(p,t)!=1)throw std::runtime_error("replay "+id+" "+std::to_string(ply+1));MoveList ml;generate_moves(p,ml);std::cout<<id<<'\t'<<++ply<<'\t'<<p.fen()<<'\t'<<p.reversible_plies()<<'\t'<<move_text(p,m)<<'\t'<<ml.size()<<'\t'<<p.piece_count()<<'\n';p.make_move(m);}}
 return 0;}
if(ac!=9)return 2;auto&g=gs.at(av[3]);int ply=std::stoi(av[4]);uint64_t budget=std::stoull(av[5]);int depth=std::stoi(av[6]);std::string force=av[7];int cap=std::stoi(av[8]);if(budget>100000000)throw std::runtime_error("budget");
if(!egdb::init("/home/admingod/regicide-data/egdb",64))throw std::runtime_error("DB");
Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");std::vector<Position>hist;
for(int i=0;i<ply-1;++i){Move m;if(!parse_move(p,g.mv.at(i),m))throw std::runtime_error("move");hist.push_back(p);p.make_move(m);}
MoveList ml;generate_moves(p,ml);
std::cout<<"# root "<<p.fen()<<" rev "<<p.reversible_plies()<<" pieces "<<p.piece_count()<<" dbcap "<<cap<<" db "<<(p.piece_count()<=cap?int(egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move())):0)<<"\n";
for(auto&m:ml){Position c=p;c.make_move(m);MoveList replies;generate_moves(c,replies);int db=c.piece_count()<=cap?int(egdb::probe(c.pieces(BLACK),c.pieces(WHITE),c.kings(),c.side_to_move())):0;std::cout<<"# child "<<move_text(p,m)<<" db "<<db<<" drawnow "<<nowdraw(p,m,hist)<<" terminal "<<replies.empty()<<"\n";}
Engine e;e.set_threads(1);e.set_hash_mb(128);e.set_egdb_pieces(cap);e.set_book(nullptr);e.set_avoid(nullptr);e.new_game();e.set_position(p,hist);
Limits l;l.nodes=budget==100000000?budget-1024:budget;l.depth=depth;l.movetime_ms=780000;l.use_book=false;
if(force!="-"){Move m;if(!parse_move(p,force,m))throw std::runtime_error("force "+force);l.searchmoves={m};l.infinite=true;}
std::cout<<"depth\tscore\tmove\tnodes\ttbhits\tpv\n";
auto r=e.go(l,[&](const SearchInfo&i){std::cout<<i.depth<<'\t'<<i.score<<'\t'<<(i.pv.empty()?"none":move_text(p,i.pv[0]))<<'\t'<<i.nodes<<'\t'<<i.tbhits<<'\t';Position q=p;for(auto m:i.pv){MoveList legal;generate_moves(q,legal);if(std::find(legal.begin(),legal.end(),m)==legal.end())throw std::runtime_error("illegal PV");std::cout<<move_text(q,m)<<' ';q.make_move(m);}std::cout<<std::endl;});
std::cout<<"# final depth "<<r.depth<<" completed "<<r.completed_score<<" score "<<r.score<<" move "<<move_text(p,r.best)<<" partial "<<r.partial<<" nodes "<<r.nodes<<" ms "<<r.time_ms<<"\n";
return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 9;}}
```

### analyze.py

```python
import csv,json,re,sys,time,subprocess
from pathlib import Path
P=Path('work/round3/task2');D=Path('reports/round3/task2/queries')
ctx={c['id']:c for c in json.load(open(P/'contexts.json'))}
jobs=json.load(open(P/'jobs.json'));statepath=P/'state.json'
state=json.loads(statepath.read_text()) if statepath.exists() else {}
def read(p):
 s=p.read_text();end=s.splitlines()[-1]
 assert end.startswith('# final'),p
 rs={int(x['depth']):x for x in csv.DictReader([s for s in s.splitlines() if not s.startswith('#')],delimiter='\t')}
 assert max(rs)==int(re.search(r'depth (\d+)',end)[1]),p
 assert int(re.search(r'nodes (\d+)',end)[1])<=100000000,p
 children={}
 for m in re.finditer(r'^# child (\S+) db (\d+) drawnow (\d+) terminal (\d+)$',s,re.M):
  children[m[1]]=dict(db=int(m[2]),drawnow=int(m[3]),terminal=int(m[4]))
 return dict(path=str(p),steps=rs,last=rs[max(rs)],children=children,final=end)
def query(cid,b,move='-',depth=127):
 c=ctx[cid];token='root' if move=='-' else 'force-'+move.replace('x','_')
 # Reuse only identical stable history contexts from the accepted round-2 cache.
 for old in Path('reports/round2/queries').glob(f'{cid}-{b}-{token}-d*.tsv'):
  if move=='-' and c['pieces']<=8:continue  # refresh table roots after PR81
  dest=D/old.name
  if not dest.exists():
   read(old)
   dest.write_text('# reused from round2 d04fbd81c801de5ff8340a96f83079873a22c7a7; same history context, n8q, 8-piece tables and unchanged search path under fdd154e\n'+old.read_text())
 # Any already completed deeper target gives the same earlier iteration prefix.
 if move!='-':
  candidates=[]
  for p in D.glob(f'{cid}-{b}-{token}-d*.tsv'):
   try:r=read(p)
   except (AssertionError,IndexError):continue
   target=int(p.stem.rsplit('-d',1)[1])
   if target>=depth:candidates.append((target,r))
  if candidates:return min(candidates,key=lambda x:x[0])[1]
 p=D/f'{cid}-{b}-{token}-d{depth}.tsv'
 if p.exists():
  try:return read(p)
  except (AssertionError,IndexError):raise RuntimeError('Incomplete query file '+str(p))
 tmp=p.with_suffix('.running')
 with tmp.open('w') as f:
  f.write('# core_revision '+(P/'current-core.txt').read_text().strip()+'\n');f.flush()
  subprocess.run(['nice','-n','19','work/round3/task2/replay','query','work/round3/task2/games.tsv',c['game'],str(c['ply']),str(b),str(depth),move,'8'],stdout=f,check=True,timeout=790)
 read(tmp);tmp.replace(p)
 return read(p)
def pair(cid,played,alt,depth):
 a=query(cid,100000000,played,max(1,depth));b=query(cid,100000000,alt,max(1,depth))
 d=min(max(1,depth),max(a['steps']),max(b['steps']))
 x,y=int(a['steps'][d]['score']),int(b['steps'][d]['score'])
 ch=b['children'].get(alt,{})
 direct=ch.get('db')==3 and not ch.get('drawnow') or bool(ch.get('terminal'))
 proven=y>=15000 and int(b['steps'][d]['tbhits'])>0
 pch=a['children'].get(played,{})
 played_direct=pch.get('db')==3 and not pch.get('drawnow') or bool(pch.get('terminal'))
 return dict(depth=d,played_score=x,alt_score=y,gain=y-x,alternative=alt,alt_direct=bool(direct),alt_engine_proven=bool(proven),played_direct=bool(played_direct),played_engine_proven=x>=15000,played_path=a['path'],alternative_path=b['path'])
def save():
 tmp=statepath.with_suffix('.tmp');tmp.write_text(json.dumps(state,indent=2));tmp.replace(statepath)
def analyze(kind,q,played):
 cid=q['context'];key=f"{kind}:{q.get('game')}:{q['ply']}:{played}"
 if key in state and state[key].get('done'):return
 c=ctx[cid]
 r=dict(kind=kind,rank=q['rank'],game=q['game'],ply=q['ply'],context=cid,fen=c['fen'],pieces=c['pieces'],played=played,node=q.get('node'),done=False)
 state[key]=r;save()
 a=query(cid,20000000);r['root20']=a['last'];r['root20_path']=a['path'];alt=a['last']['move']
 if c['legal']==1:
  r.update(status='only legal move',done=True);save();print(key,r['status'],flush=True);return
 if alt==played:
  r.update(status='20M chooses played move',done=True);save();print(key,r['status'],flush=True);return
 # Every differing 20M choice gets a 100M root check. Run each restricted move
 # only once to the deeper reference; its completed trace supplies both depths.
 b=query(cid,100000000);r['root100']=b['last'];r['root100_path']=b['path'];save()
 alt100=b['last']['move']
 if alt100!=played:alt=alt100
 z=pair(cid,played,alt,max(int(a['last']['depth']),int(b['last']['depth'])));r['pair100']=z
 # Compare precisely the same alternative at both references, using its deeper run's prefix.
 p=pair(cid,played,alt,int(a['last']['depth']));r['pair20']=p
 valueclass=lambda s:1 if s>=15000 else -1 if s<=-15000 else 0
 ordinary=lambda v:(v['gain']>30 if valueclass(v['played_score'])==valueclass(v['alt_score'])==0 else valueclass(v['alt_score'])>valueclass(v['played_score']))
 margin=ordinary(p) and ordinary(z) and alt100!=played
 proven=p['alt_direct'] or z['alt_direct'] or p['alt_engine_proven'] or z['alt_engine_proven']
 stable=p['alt_direct'] or z['alt_direct'] or z['alt_engine_proven']
 both=(z['played_direct'] or z['played_engine_proven']) and stable
 r.update(alternative=alt,qualifies=bool(margin or proven),clear_margin=bool(margin),proven_alternative=bool(proven),proof_stable=bool(stable),unstable_proof=bool(proven and not stable),both_moves_win=bool(both),strict_missed_win=bool(stable and not both),status='qualifies' if margin or proven else 'does not qualify at both references',done=True)
 save();print(key,r['status'],'played',played,'alt',alt,'pairs',p['depth'],p['played_score'],p['alt_score'],z['depth'],z['played_score'],z['alt_score'],'proven',proven,'both',both,flush=True)
if __name__=='__main__':
 kind=sys.argv[1];lo,hi=map(int,sys.argv[2:4]);start=time.monotonic();processed=0
 for q in jobs[kind]:
  if not lo<=q['rank']<=hi:continue
  moves=q['losing_moves'] if kind=='mining' else [q['played']]
  for played in moves:
   analyze(kind,q,played);processed+=1
   if time.monotonic()-start>360:
    print('BATCH TIME CHECKPOINT',kind,lo,hi,processed,flush=True);sys.exit(0)
 print('BATCH RANGE DONE',kind,lo,hi,flush=True)
```

### mining.py

```python
import json,sys,time
from pathlib import Path
import analyze as A
lo,hi=map(int,sys.argv[1:3]);started=time.monotonic()
for q in A.jobs['mining']:
 if not lo<=q['rank']<=hi:continue
 cid=q['context'];c=A.ctx[cid];tag='mine:'+q['node']
 if A.state.get(tag,{}).get('done'):continue
 a=A.query(cid,20000000);b=A.query(cid,100000000)
 roots=[a['last']['move'],b['last']['move']]
 moves=sorted(set(q['losing_moves']+q['drawing_moves']+roots))
 depth=max(1,int(a['last']['depth']),int(b['last']['depth']))
 for move in moves:A.query(cid,100000000,move,depth)
 results=[]
 for played in q['losing_moves']:
  alternatives=sorted(set(moves)-{played})
  for alt in alternatives:
   p=A.pair(cid,played,alt,int(a['last']['depth']));z=A.pair(cid,played,alt,int(b['last']['depth']))
   cls=lambda v:1 if v>=15000 else -1 if v<=-15000 else 0
   better=lambda r:r['gain']>30 if cls(r['played_score'])==cls(r['alt_score'])==0 else cls(r['alt_score'])>cls(r['played_score'])
   margin=better(p) and better(z)
   proof=p['alt_direct'] or z['alt_direct'] or p['alt_engine_proven'] or z['alt_engine_proven']
   stable=p['alt_direct'] or z['alt_direct'] or z['alt_engine_proven']
   both=stable and (z['played_direct'] or z['played_engine_proven'])
   results.append(dict(played=played,alternative=alt,pair20=p,pair100=z,clear_margin=bool(margin),proven_alternative=bool(proof),proof_stable=bool(stable),unstable_proof=bool(proof and not stable),both_moves_win=bool(both),qualifies=bool(margin or proof),root_consistent=all(x!=played for x in roots),observed_drawing_move=alt in q['drawing_moves']))
 r=dict(kind='mining',rank=q['rank'],node=q['node'],game=q['game'],ply=q['ply'],context=cid,fen=c['fen'],pieces=c['pieces'],root20=a['last'],root100=b['last'],root20_path=a['path'],root100_path=b['path'],comparisons=results,done=True)
 A.state[tag]=r;A.save()
 print(q['rank'],q['node'],'choices',moves,'qualified',[(x['played'],x['alternative'],x['pair20']['gain'],x['pair100']['gain'],x['root_consistent']) for x in results if x['qualifies']],flush=True)
 if time.monotonic()-started>360:
  print('BATCH TIME CHECKPOINT mining',lo,hi,flush=True);sys.exit(0)
print('BATCH RANGE DONE mining',lo,hi,flush=True)
```
