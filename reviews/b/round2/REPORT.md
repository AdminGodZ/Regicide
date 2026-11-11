# Reviewer B, round 2

2026-11-11 11:03 UTC

Status: completed bounded round; unprocessed queues listed below

## Scope and results

This is a bounded review of the highest in-game-score draws, not an exhaustive pass over all selected games. All engine work stayed in /home/admingod/regicide-review-b, at nice -n 19, one search at a time on one thread, hash 128 MiB freshly cleared per query, with book/avoid off and full record history replayed. No match was played and no other agent process, lock or working directory was changed. The shared database was only read.

| Source | Reviewed input commit |
|---|---|
| origin/master | 8de39d0ca7db3d586cf92b56df25b06a4ae21d81 |
| origin/t3/booklab | 223e6af72d2048d9841ddd1853bdf8d692f93aa2 |
| origin/t3/review-reports mining input | e81396c188c551cb817ac7e40cb3577626d32042 |

Final fetch re-check: master 8de39d0ca7db3d586cf92b56df25b06a4ae21d81, booklab 90f031c3ff1c9676ac903bbf098f1db6d961b005. Core/egdb build inputs and embedded assets are identical to the measured core; the later src/cb_api.cpp loss-claim change belongs only to the CheckerBoard DLL, which is not linked into the driver, and every requested validation/match-order PDN path is unchanged from the input commit above. The latest ledger was read again. Later targeted process-state experiments are outside this round’s requested input pool.

The committed corpus contains 7944 finished, legally replayed games (6352 against the requested Cake/KingsRow opponents). There are 116 drawn games with an actual own search comment at +40 or higher: 68 Cake and 48 KingsRow. The walk has 1415 own positions before the cutoff (1078 with more than one legal move), plus 67 own-turn first-database endpoints. A game with no <=8 position is walked through its recorded end. All 112 recorded first-database endpoints are draws; the other 4 selected games end above eight pieces.

Completed consecutive highest-score prefix: **20 of 116 games**; 155 own-move positions checked, 18 differing choices compared at both references. Opponent predecessors checked: 20 of 116, including 8 full two-reference comparisons. Mining-list coverage: **first 12 of 36 ordered entries**; 12 complete entries overall. The largest entry (881.24, ACF 71) reuses four verified current-engine round-1 traces and is not a new finding.

Qualifying own contexts: 0; qualifying predecessor contexts: 0; new mining contexts with both unrestricted roots rejecting the losing-game move: 1. All qualifying alternatives, including any case where both moves retain a proven win, are listed below. Search opinions outside directly probed database children are not independent game-theoretic proofs.

The highest in-game peak is +214 in R4102 (KingsRow, ACF 16, Regicide Black, round 14.1). Its first trigger is ply 105; our moves at plies 105, 107 and 109 each have only one legal choice, and the first database board at ply 111 is drawn. Another high peak, +169 in R946 (KingsRow, ACF 5, Regicide Black, round 4.1), has the same shape: forced own moves at 57, 59 and 61, then a drawn database board at 63. These late positive comments do not identify an unplayed winning choice in the checked walk.

## Selection and stopping point

Only Regicide search comments are used, from the moving side’s point of view. Ballot moves, absent scores, and depth-0/zero-node book/table replies are not triggers. Validation source names contain the recorded n6c/n8q configuration; all re-analysis uses the current built-in n8q. Drawn games are sorted by descending highest own search score, then source and round for ties. The mining list excludes the three within-configuration contrasts and sorts the remaining 36 by game count descending, then numeric node ID. There are no duplicate source/round/colour identities. Frequencies include all finished results, not just the selected draws.

Next own game in priority order: rank 21, wp11c/games/bl-v11c-n6c-cake-a-h1.pdn.gz, round 64.1, ACF #72, Regicide B, peak +98, first +40 at ply 37.

Next mining entry: rank 13, node 422.17, ACF #153 Black Widow, guinn, Regicide B, ply 18, 3 input games.

The match-order PDNs were committed during this review under tests/mo-*/games.pdn.gz, and the completed r11d/n8q validation records under tests/v11d-n8q-*/games.pdn.gz. Both were added. Original game IDs and history-context cache identities were preserved, and run/round/colour identities were checked to avoid duplicate archived copies. The input pool is the requested wp11c/wp11d validation family plus match-order runs; separate targeted E2 trials are outside this pool. Result CSVs/prose were not substituted for PDNs. The full selection/backlog is in selected-games.tsv and game-coverage.tsv; mining-coverage.tsv records every unprocessed entry.

| Rank | Record | Opening | Ours | Peak | First +40 | Own checked/requested | Predecessor |
|---|---|---|---|---|---|---|---|
| 1 | R4102 / 14.1 | ACF #16 Wilderness II | B | 214 | 105 / 214 | 4/4 | True |
| 2 | R2794 / 68.2 | ACF #162 | W | 186 | 46 / 186 | 4/4 | True |
| 3 | R4329 / 57.2 | ACF #150 | W | 177 | 30 / 84 | 13/13 | True |
| 4 | R1762 / 20.2 | ACF #110 | W | 174 | 32 / 53 | 11/11 | True |
| 5 | R946 / 4.1 | ACF #5 Dreaded Edinburgh | B | 169 | 57 / 169 | 4/4 | True |
| 6 | R3710 / 59.1 | ACF #152 | B | 152 | 35 / 152 | 10/10 | True |
| 7 | R6637 / 46.1 | ACF #138 | B | 143 | 39 / 143 | 16/16 | True |
| 8 | R7895 / 55.1 | ACF #148 | B | 140 | 37 / 90 | 7/7 | True |
| 9 | R3312 / 13.1 | ACF #102 | B | 139 | 49 / 139 | 10/10 | True |
| 10 | R6368 / 74.1 | ACF #169 | B | 138 | 33 / 138 | 5/5 | True |
| 11 | R513 / 20.2 | ACF #110 | W | 138 | 32 / 72 | 11/11 | True |
| 12 | R1855 / 66.2 | ACF #160 | W | 121 | 42 / 111 | 11/11 | True |
| 13 | R4515 / 71.2 | ACF #80 | W | 121 | 40 / 121 | 7/7 | True |
| 14 | R1336 / 42.2 | ACF #49 | W | 118 | 52 / 118 | 4/4 | True |
| 15 | R1713 / 78.1 | ACF #87 Tyne | B | 117 | 25 / 117 | 11/11 | True |
| 16 | R3319 / 20.2 | ACF #110 | W | 114 | 38 / 114 | 8/8 | True |
| 17 | R5087 / 54.2 | ACF #147 | W | 112 | 44 / 112 | 4/4 | True |
| 18 | R5403 / 54.2 | ACF #147 | W | 112 | 44 / 112 | 4/4 | True |
| 19 | R6649 / 54.2 | ACF #147 | W | 112 | 44 / 112 | 4/4 | True |
| 20 | R7614 / 63.2 | ACF #157 | W | 111 | 48 / 111 | 7/7 | True |

## Search and comparison method

The Release core built at 1c6e997 was reused after the final fetch showed no core/egdb source, header, CMake target or built-in-asset differences against the reviewed current master. Later changes affect ledger/runner tests, the match tool and the separate CheckerBoard DLL. The file cb_api.cpp is not part of the static core linked into this driver. The round-2 drivers were compiled with Clang 22, -O3 -march=native, against that exact core. Built-in net n8q SHA-256 is 681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24. WLD coverage is eight pieces and MTC coverage six. Budgets are nominal 20M/100M; 100M is reduced by 1,024 internally to stay below the strict actual ceiling. The independent time ceiling is 780 seconds; no reported query reaches it. Single-legal-move and exact-table cases may finish before using the budget.

Root choices/scores are the last completed iteration, with the actual final partial status retained in each raw file. When 20M chooses a different own move, 100M is run even if the shallow margin is small. Each forced move is searched to the deeper reference once where possible; its completed iteration trace provides both reference depths. A pair uses the largest depth completed for both moves if one cannot reach the unrestricted reference. The same alternative is compared at both references. Root/forced differences caused by selective search are shown, not hidden.

A numeric improvement requires >30 at both references, with the 100M root still rejecting the recorded move for own/predecessor checks. Mate/database score classes are compared by value class rather than treating distance differences as hundredths of a man. An engine-known database win is score >=15000 with database use in that restricted run; a newly found 100M win is reported even if 20M did not prove it, and a shallow known-win score that disappears at 100M is flagged as unstable rather than counted as a confirmed win; a directly probed winning child is separately identified and checked against immediate repetition/no-progress draws and terminal legality. When both moves retain a win, that is reported separately and is not counted as a strict missed winning move.

For mining contrasts, every move observed in the supplied mining evidence is forced, together with any additional unrestricted best move, even if the unrestricted engine chooses the losing-game move. All losing-game moves are compared with the other tested choices. The table separates forced-pair qualification from agreement of both unrestricted roots. A root-disagreement case is not silently promoted into a confirmed core error.

Completed raw query files: 278 (4 reused from round 1, 274 new); maximum actual nodes 99,999,748; longest query 45.159 seconds. Every reported query has a final result and stayed within the node/time ceiling. See query-manifest.tsv and queries/.

## Qualifying positions

Scores in each pair are played / alternative, from the mover’s point of view. FEN includes side to move. Records are grouped only when board, reversible count and relevant history tail agree.

| Point | Kind | Opening | Ours | Opponent | Ply | FEN | Played | Alternative | 20M reference | 100M reference | Proven alternative | Both win | Roots agree |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Q001 | mining | ACF #71 Diabolical Denny | W | kingsrow | 25 | B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13 | 10-14 | 5-9 | -188 / -3 at d27 (gain 185) | -186 / 0 at d32 (gain 186) | False | False | True |
| Q002 | mining | ACF #141 | W | guinn | 13 | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 16-20 | 11-15 | -189 / -72 at d24 (gain 117) | -189 / -56 at d25 (gain 133) | False | False | True |
| Q003 | mining | ACF #141 | W | guinn | 13 | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 4-8 | 11-15 | -167 / -72 at d24 (gain 95) | -167 / -37 at d27 (gain 130) | False | False | True |

The new board is ACF 141 with GuiNN Black to move at full-game ply 13 (input node 399.12), Regicide White. Both unrestricted roots choose 11-15: -36 at depth 24 with the 20M budget and -64 at depth 28 with 100M. In the restricted comparisons, 16-20 trails 11-15 by 117/133, and 4-8 trails by 95/130. The 100M common completed depths are 25 and 27 respectively; the unrestricted reference reaches 28, but the losing moves do not both complete that depth within the cap. No move is database-proven here. In the reviewed pool, 16-20 was played twice and 4-8 twice, all four games won by Regicide; 11-15 was played four times, with one Regicide win and three draws. This extends the old cross-configuration contrast, but does not establish that 11-15 draws with best play.

## Frequencies at qualifying boards

The census uses all 7,944 committed games of the reviewed validation/match-order pool, including wins and losses. For own points, the primary counts use Cake/KingsRow games where Regicide is the mover; for opponent points, they use that opponent and the opposite Regicide side. Full boards are matched across move-order transpositions and opening names. Distinct games and total visits are separate: repeated king positions in one game do not become several games. A board census does not imply identical reversible history or identical search evaluation at every visit. Outcomes are from Regicide’s side. All per-game visits are in qualifying-occurrences.tsv.

| Point | Kind | FEN | All games | All visits | Move | Games with move | Visits with move | Our W | D | L |
|---|---|---|---|---|---|---|---|---|---|---|
| Q001 | mining | B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13 | 7 | 7 | 10-14 | 1 | 1 | 1 | 0 | 0 |
| Q001 | mining | B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13 | 7 | 7 | 5-9 | 6 | 6 | 0 | 6 | 0 |
| Q002 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 11-15 | 4 | 4 | 1 | 3 | 0 |
| Q002 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 16-20 | 2 | 2 | 2 | 0 | 0 |
| Q002 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 4-8 | 2 | 2 | 2 | 0 | 0 |
| Q003 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 11-15 | 4 | 4 | 1 | 3 | 0 |
| Q003 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 16-20 | 2 | 2 | 2 | 0 | 0 |
| Q003 | mining | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 8 | 8 | 4-8 | 2 | 2 | 2 | 0 | 0 |

## Complete own-move and predecessor results

These tables retain negative checks as well as qualified ones. A dash in the 100M/pair columns means the 20M choice was already the recorded move, or there was only one legal move; no alternative was selected by this protocol. It does not mean a separate 100M search was run.

### Own moves

| Rank | Game / ply | Played | 20M root | 100M root | 20M pair | 100M pair | Status |
|---|---|---|---|---|---|---|---|
| 1 | R4102 / 105 | 2x11 | 2x11 / 309 / d1 | - | - | - | only legal move |
| 1 | R4102 / 107 | 11x18 | 11x18 / 0 / d1 | - | - | - | only legal move |
| 1 | R4102 / 109 | 12x19 | 12x19 / 0 / d1 | - | - | - | only legal move |
| 1 | R4102 / 111 | 30-25 | 30-25 / 0 / d127 | - | - | - | 20M chooses played move |
| 2 | R2794 / 46 | 22x13 | 22x13 / 186 / d1 | - | - | - | only legal move |
| 2 | R2794 / 48 | 25-21 | 15-11 / 0 / d127 | 15-11 / 0 / d127 | 0 / 0 at d75 (gain 0) | 0 / 0 at d75 (gain 0) | does not qualify at both references |
| 2 | R2794 / 50 | 27-23 | 27-23 / 0 / d127 | - | - | - | 20M chooses played move |
| 2 | R2794 / 52 | 15-10 | 15-10 / 0 / d127 | - | - | - | 20M chooses played move |
| 3 | R4329 / 30 | 20x11 | 20x11 / 150 / d1 | - | - | - | only legal move |
| 3 | R4329 / 32 | 9-5 | 9-5 / 27 / d27 | - | - | - | 20M chooses played move |
| 3 | R4329 / 34 | 18-15 | 18-15 / 0 / d29 | - | - | - | 20M chooses played move |
| 3 | R4329 / 36 | 15-11 | 15-11 / 8 / d30 | - | - | - | 20M chooses played move |
| 3 | R4329 / 38 | 30-25 | 30-25 / 0 / d32 | - | - | - | 20M chooses played move |
| 3 | R4329 / 40 | 26x19 | 26x19 / 203 / d1 | - | - | - | only legal move |
| 3 | R4329 / 42 | 21x14 | 21x14 / 141 / d1 | - | - | - | only legal move |
| 3 | R4329 / 44 | 19-15 | 19-16 / 0 / d38 | 19-16 / 0 / d46 | 0 / 0 at d38 (gain 0) | 0 / 0 at d46 (gain 0) | does not qualify at both references |
| 3 | R4329 / 46 | 25-21 | 15-10 / 0 / d48 | 15-10 / 0 / d56 | 0 / 0 at d48 (gain 0) | 0 / 0 at d55 (gain 0) | does not qualify at both references |
| 3 | R4329 / 48 | 31x24 | 31x24 / 66 / d1 | - | - | - | only legal move |
| 3 | R4329 / 50 | 15-10 | 15-10 / 0 / d54 | - | - | - | 20M chooses played move |
| 3 | R4329 / 52 | 10-6 | 11-7 / 0 / d53 | 11-7 / 0 / d62 | 0 / 0 at d53 (gain 0) | 0 / 0 at d62 (gain 0) | does not qualify at both references |
| 3 | R4329 / 54 | 11-7 | 11-7 / 0 / d127 | - | - | - | 20M chooses played move |
| 4 | R1762 / 32 | 26x19 | 26x19 / 53 / d1 | - | - | - | only legal move |
| 4 | R1762 / 34 | 17-13 | 17-13 / 0 / d28 | - | - | - | 20M chooses played move |
| 4 | R1762 / 36 | 30-26 | 30-26 / 0 / d28 | - | - | - | 20M chooses played move |
| 4 | R1762 / 38 | 31-27 | 31-27 / 0 / d31 | - | - | - | 20M chooses played move |
| 4 | R1762 / 40 | 16x7 | 16x7 / 174 / d1 | - | - | - | only legal move |
| 4 | R1762 / 42 | 26-22 | 26-22 / 0 / d31 | - | - | - | 20M chooses played move |
| 4 | R1762 / 44 | 27x18 | 27x18 / 11 / d1 | - | - | - | only legal move |
| 4 | R1762 / 46 | 20-16 | 21-17 / 0 / d35 | 21-17 / 0 / d41 | 0 / 0 at d35 (gain 0) | 0 / 0 at d41 (gain 0) | does not qualify at both references |
| 4 | R1762 / 48 | 28-24 | 28-24 / 0 / d55 | - | - | - | 20M chooses played move |
| 4 | R1762 / 50 | 16-11 | 16-11 / 0 / d127 | - | - | - | 20M chooses played move |
| 4 | R1762 / 52 | 19x10x1 | 19x10x1 / 0 / d1 | - | - | - | only legal move |
| 5 | R946 / 57 | 16x23 | 16x23 / 179 / d1 | - | - | - | only legal move |
| 5 | R946 / 59 | 13x22 | 13x22 / 0 / d1 | - | - | - | only legal move |
| 5 | R946 / 61 | 5x14 | 5x14 / 0 / d1 | - | - | - | only legal move |
| 5 | R946 / 63 | 31-26 | 31-26 / 0 / d127 | - | - | - | 20M chooses played move |
| 6 | R3710 / 35 | 2x9 | 2x9 / 64 / d1 | - | - | - | only legal move |
| 6 | R3710 / 37 | 12-16 | 10-15 / 0 / d29 | 10-15 / 0 / d34 | 0 / 0 at d29 (gain 0) | 0 / 0 at d34 (gain 0) | does not qualify at both references |
| 6 | R3710 / 39 | 16-20 | 16-20 / 0 / d30 | - | - | - | 20M chooses played move |
| 6 | R3710 / 41 | 10-15 | 10-15 / 0 / d33 | - | - | - | 20M chooses played move |
| 6 | R3710 / 43 | 24-27 | 15-18 / 0 / d33 | 15-18 / 0 / d38 | 0 / 0 at d33 (gain 0) | 0 / 0 at d38 (gain 0) | does not qualify at both references |
| 6 | R3710 / 45 | 20x27 | 20x27 / -10 / d1 | - | - | - | only legal move |
| 6 | R3710 / 47 | 27-31 | 27-31 / 0 / d43 | - | - | - | 20M chooses played move |
| 6 | R3710 / 49 | 31-27 | 31-27 / 0 / d46 | - | - | - | 20M chooses played move |
| 6 | R3710 / 51 | 14x23 | 14x23 / 0 / d1 | - | - | - | only legal move |
| 6 | R3710 / 53 | 27-24 | 27-24 / 0 / d127 | - | - | - | 20M chooses played move |
| 7 | R6637 / 39 | 6x15 | 6x15 / 143 / d1 | - | - | - | only legal move |
| 7 | R6637 / 41 | 24-28 | 24-28 / 0 / d29 | - | - | - | 20M chooses played move |
| 7 | R6637 / 43 | 19-24 | 19-24 / 0 / d30 | - | - | - | 20M chooses played move |
| 7 | R6637 / 45 | 15-19 | 15-19 / 0 / d31 | - | - | - | 20M chooses played move |
| 7 | R6637 / 47 | 18-23 | 18-23 / 0 / d31 | - | - | - | 20M chooses played move |
| 7 | R6637 / 49 | 19-23 | 19-23 / 0 / d30 | - | - | - | 20M chooses played move |
| 7 | R6637 / 51 | 2x9 | 2x9 / 9 / d1 | - | - | - | only legal move |
| 7 | R6637 / 53 | 24-27 | 24-27 / 0 / d31 | - | - | - | 20M chooses played move |
| 7 | R6637 / 55 | 20x27 | 20x27 / 0 / d1 | - | - | - | only legal move |
| 7 | R6637 / 57 | 27-31 | 27-31 / 0 / d30 | - | - | - | 20M chooses played move |
| 7 | R6637 / 59 | 31-26 | 31-26 / 0 / d31 | - | - | - | 20M chooses played move |
| 7 | R6637 / 61 | 16-20 | 16-20 / 0 / d29 | - | - | - | 20M chooses played move |
| 7 | R6637 / 63 | 23-27 | 23-27 / 0 / d36 | - | - | - | 20M chooses played move |
| 7 | R6637 / 65 | 26x19 | 26x19 / 0 / d1 | - | - | - | only legal move |
| 7 | R6637 / 67 | 1x10 | 1x10 / 0 / d1 | - | - | - | only legal move |
| 7 | R6637 / 69 | 19-15 | 19-15 / 0 / d127 | - | - | - | 20M chooses played move |
| 8 | R7895 / 37 | 15x22 | 15x22 / 90 / d1 | - | - | - | only legal move |
| 8 | R7895 / 39 | 2x11 | 2x11 / 52 / d1 | - | - | - | only legal move |
| 8 | R7895 / 41 | 22-25 | 22-25 / 0 / d39 | - | - | - | 20M chooses played move |
| 8 | R7895 / 43 | 19x26 | 19x26 / 140 / d1 | - | - | - | only legal move |
| 8 | R7895 / 45 | 4-8 | 25-30 / 0 / d127 | 25-30 / 0 / d127 | 0 / 0 at d127 (gain 0) | 0 / 0 at d127 (gain 0) | does not qualify at both references |
| 8 | R7895 / 47 | 25-29 | 25-30 / 0 / d127 | 25-30 / 0 / d127 | 0 / 0 at d127 (gain 0) | 0 / 0 at d127 (gain 0) | does not qualify at both references |
| 8 | R7895 / 49 | 5x14 | 5x14 / 0 / d1 | - | - | - | only legal move |
| 9 | R3312 / 49 | 26x19 | 26x19 / 160 / d1 | - | - | - | only legal move |
| 9 | R3312 / 51 | 15-18 | 15-18 / 0 / d36 | - | - | - | 20M chooses played move |
| 9 | R3312 / 53 | 19-24 | 19-23 / 0 / d39 | 19-23 / 0 / d45 | 0 / 0 at d39 (gain 0) | 0 / 0 at d41 (gain 0) | does not qualify at both references |
| 9 | R3312 / 55 | 9-14 | 9-14 / 0 / d34 | - | - | - | 20M chooses played move |
| 9 | R3312 / 57 | 14-18 | 14-18 / 0 / d32 | - | - | - | 20M chooses played move |
| 9 | R3312 / 59 | 18x27 | 18x27 / 74 / d1 | - | - | - | only legal move |
| 9 | R3312 / 61 | 10-15 | 10-15 / 0 / d44 | - | - | - | 20M chooses played move |
| 9 | R3312 / 63 | 28-32 | 28-32 / 0 / d46 | - | - | - | 20M chooses played move |
| 9 | R3312 / 65 | 32-27 | 32-27 / 0 / d127 | - | - | - | 20M chooses played move |
| 9 | R3312 / 67 | 27x18 | 27x18 / 0 / d1 | - | - | - | only legal move |
| 10 | R6368 / 33 | 14x21 | 14x21 / 138 / d1 | - | - | - | only legal move |
| 10 | R6368 / 35 | 19x26 | 19x26 / 0 / d55 | - | - | - | 20M chooses played move |
| 10 | R6368 / 37 | 21x30 | 21x30 / 107 / d1 | - | - | - | only legal move |
| 10 | R6368 / 39 | 15-18 | 8-12 / 0 / d64 | 8-12 / 0 / d76 | 0 / 0 at d64 (gain 0) | 0 / 0 at d76 (gain 0) | does not qualify at both references |
| 10 | R6368 / 41 | 18-22 | 18-22 / 0 / d127 | - | - | - | 20M chooses played move |
| 11 | R513 / 32 | 26x19 | 26x19 / 53 / d1 | - | - | - | only legal move |
| 11 | R513 / 34 | 17-13 | 17-13 / 0 / d28 | - | - | - | 20M chooses played move |
| 11 | R513 / 36 | 30-26 | 30-26 / 0 / d28 | - | - | - | 20M chooses played move |
| 11 | R513 / 38 | 31-27 | 31-27 / 0 / d31 | - | - | - | 20M chooses played move |
| 11 | R513 / 40 | 16x7 | 16x7 / 174 / d1 | - | - | - | only legal move |
| 11 | R513 / 42 | 26-22 | 26-22 / 0 / d31 | - | - | - | 20M chooses played move |
| 11 | R513 / 44 | 27x18 | 27x18 / 11 / d1 | - | - | - | only legal move |
| 11 | R513 / 46 | 28-24 | 21-17 / 0 / d35 | 21-17 / 0 / d41 | 0 / 0 at d35 (gain 0) | 0 / 0 at d41 (gain 0) | does not qualify at both references |
| 11 | R513 / 48 | 20-16 | 20-16 / 0 / d40 | - | - | - | 20M chooses played move |
| 11 | R513 / 50 | 16-11 | 16-11 / 0 / d127 | - | - | - | 20M chooses played move |
| 11 | R513 / 52 | 19x10x1 | 19x10x1 / 0 / d1 | - | - | - | only legal move |
| 12 | R1855 / 42 | 31x24 | 31x24 / 111 / d1 | - | - | - | only legal move |
| 12 | R1855 / 44 | 30x23 | 30x23 / 121 / d1 | - | - | - | only legal move |
| 12 | R1855 / 46 | 29-25 | 16-11 / 0 / d33 | 16-11 / 0 / d39 | 0 / 0 at d33 (gain 0) | 0 / 0 at d37 (gain 0) | does not qualify at both references |
| 12 | R1855 / 48 | 25-21 | 24-20 / 0 / d32 | 24-20 / 0 / d37 | 0 / 0 at d32 (gain 0) | 0 / 0 at d37 (gain 0) | does not qualify at both references |
| 12 | R1855 / 50 | 16-11 | 16-11 / 0 / d34 | - | - | - | 20M chooses played move |
| 12 | R1855 / 52 | 24-20 | 24-20 / 0 / d33 | - | - | - | 20M chooses played move |
| 12 | R1855 / 54 | 14-10 | 14-10 / 0 / d35 | - | - | - | 20M chooses played move |
| 12 | R1855 / 56 | 10-6 | 10-6 / 0 / d37 | - | - | - | 20M chooses played move |
| 12 | R1855 / 58 | 1-6 | 1-6 / 0 / d38 | - | - | - | 20M chooses played move |
| 12 | R1855 / 60 | 11-7 | 20-16 / 0 / d37 | 20-16 / 0 / d43 | 0 / 0 at d37 (gain 0) | 0 / 0 at d43 (gain 0) | does not qualify at both references |
| 12 | R1855 / 62 | 6x15 | 6x15 / 0 / d1 | - | - | - | only legal move |
| 13 | R4515 / 40 | 19x12 | 19x12 / 159 / d1 | - | - | - | only legal move |
| 13 | R4515 / 42 | 18-14 | 18-14 / 0 / d37 | - | - | - | 20M chooses played move |
| 13 | R4515 / 44 | 27x18 | 27x18 / 31 / d1 | - | - | - | only legal move |
| 13 | R4515 / 46 | 28-24 | 12-8 / 0 / d127 | 12-8 / 0 / d127 | 0 / 0 at d70 (gain 0) | 0 / 0 at d70 (gain 0) | does not qualify at both references |
| 13 | R4515 / 48 | 24-19 | 24-19 / 0 / d127 | - | - | - | 20M chooses played move |
| 13 | R4515 / 50 | 19-16 | 19-16 / 0 / d127 | - | - | - | 20M chooses played move |
| 13 | R4515 / 52 | 16x7 | 16x7 / 0 / d1 | - | - | - | only legal move |
| 14 | R1336 / 52 | 22x15 | 22x15 / 134 / d1 | - | - | - | only legal move |
| 14 | R1336 / 54 | 6-9 | 6-9 / 0 / d56 | - | - | - | 20M chooses played move |
| 14 | R1336 / 56 | 30-25 | 30-25 / 0 / d127 | - | - | - | 20M chooses played move |
| 14 | R1336 / 58 | 9-5 | 9-5 / 0 / d127 | - | - | - | 20M chooses played move |
| 15 | R1713 / 25 | 20x27 | 20x27 / 117 / d1 | - | - | - | only legal move |
| 15 | R1713 / 27 | 1-6 | 1-6 / 0 / d29 | - | - | - | 20M chooses played move |
| 15 | R1713 / 29 | 7x16 | 7x16 / 123 / d1 | - | - | - | only legal move |
| 15 | R1713 / 31 | 3-7 | 3-7 / 0 / d30 | - | - | - | 20M chooses played move |
| 15 | R1713 / 33 | 16-19 | 16-19 / 0 / d35 | - | - | - | 20M chooses played move |
| 15 | R1713 / 35 | 12x19 | 12x19 / 55 / d1 | - | - | - | only legal move |
| 15 | R1713 / 37 | 7-10 | 7-10 / 0 / d44 | - | - | - | 20M chooses played move |
| 15 | R1713 / 39 | 2x11 | 2x11 / -232 / d1 | - | - | - | only legal move |
| 15 | R1713 / 41 | 6-10 | 6-10 / 0 / d45 | - | - | - | 20M chooses played move |
| 15 | R1713 / 43 | 9-14 | 9-14 / 0 / d127 | - | - | - | 20M chooses played move |
| 15 | R1713 / 45 | 5x14x21 | 5x14x21 / 0 / d1 | - | - | - | only legal move |
| 16 | R3319 / 38 | 16x7 | 16x7 / 36 / d1 | - | - | - | only legal move |
| 16 | R3319 / 40 | 31-27 | 31-27 / 0 / d30 | - | - | - | 20M chooses played move |
| 16 | R3319 / 42 | 26-22 | 26-22 / 0 / d31 | - | - | - | 20M chooses played move |
| 16 | R3319 / 44 | 27x18 | 27x18 / 11 / d1 | - | - | - | only legal move |
| 16 | R3319 / 46 | 20-16 | 21-17 / 0 / d35 | 21-17 / 0 / d41 | 0 / 0 at d35 (gain 0) | 0 / 0 at d41 (gain 0) | does not qualify at both references |
| 16 | R3319 / 48 | 28-24 | 28-24 / 0 / d55 | - | - | - | 20M chooses played move |
| 16 | R3319 / 50 | 16-11 | 16-11 / 0 / d127 | - | - | - | 20M chooses played move |
| 16 | R3319 / 52 | 19x10x1 | 19x10x1 / 0 / d1 | - | - | - | only legal move |
| 17 | R5087 / 44 | 32x23 | 32x23 / 112 / d1 | - | - | - | only legal move |
| 17 | R5087 / 46 | 18-14 | 18-14 / 0 / d127 | - | - | - | 20M chooses played move |
| 17 | R5087 / 48 | 14-9 | 14-9 / 0 / d127 | - | - | - | 20M chooses played move |
| 17 | R5087 / 50 | 17x10 | 17x10 / 0 / d1 | - | - | - | only legal move |
| 18 | R5403 / 44 | 32x23 | 32x23 / 112 / d1 | - | - | - | only legal move |
| 18 | R5403 / 46 | 18-14 | 18-14 / 0 / d127 | - | - | - | 20M chooses played move |
| 18 | R5403 / 48 | 14-9 | 14-9 / 0 / d127 | - | - | - | 20M chooses played move |
| 18 | R5403 / 50 | 17x10 | 17x10 / 0 / d1 | - | - | - | only legal move |
| 19 | R6649 / 44 | 32x23 | 32x23 / 112 / d1 | - | - | - | only legal move |
| 19 | R6649 / 46 | 18-14 | 18-14 / 0 / d127 | - | - | - | 20M chooses played move |
| 19 | R6649 / 48 | 14-9 | 14-9 / 0 / d127 | - | - | - | 20M chooses played move |
| 19 | R6649 / 50 | 17x10 | 17x10 / 0 / d1 | - | - | - | only legal move |
| 20 | R7614 / 48 | 16x7 | 16x7 / 111 / d1 | - | - | - | only legal move |
| 20 | R7614 / 50 | 7-2 | 7-2 / 0 / d34 | - | - | - | 20M chooses played move |
| 20 | R7614 / 52 | 2-7 | 3-7 / 0 / d35 | 3-7 / 0 / d42 | 0 / 0 at d35 (gain 0) | 0 / 0 at d42 (gain 0) | does not qualify at both references |
| 20 | R7614 / 54 | 7-10 | 7-10 / 0 / d38 | - | - | - | 20M chooses played move |
| 20 | R7614 / 56 | 10-14 | 10-14 / 0 / d39 | - | - | - | 20M chooses played move |
| 20 | R7614 / 58 | 27x18 | 27x18 / 0 / d1 | - | - | - | only legal move |
| 20 | R7614 / 60 | 28-24 | 28-24 / 0 / d127 | - | - | - | 20M chooses played move |

### Opponent immediately before first +40

| Rank | Game / ply | Played | 20M root | 100M root | 20M pair | 100M pair | Status |
|---|---|---|---|---|---|---|---|
| 1 | R4102 / 104 | 3-7 | 10-15 / 0 / d28 | 10-15 / 0 / d33 | 0 / 0 at d28 (gain 0) | 0 / 0 at d33 (gain 0) | does not qualify at both references |
| 2 | R2794 / 45 | 13-17 | 13-17 / 0 / d61 | - | - | - | 20M chooses played move |
| 3 | R4329 / 29 | 12-16 | 24-28 / -19 / d26 | 24-28 / 0 / d31 | -8 / -7 at d26 (gain 1) | 0 / -8 at d31 (gain -8) | does not qualify at both references |
| 4 | R1762 / 31 | 19-23 | 19-23 / 0 / d28 | - | - | - | 20M chooses played move |
| 5 | R946 / 56 | 23-19 | 21-17 / 0 / d35 | 21-17 / 0 / d39 | 0 / 0 at d35 (gain 0) | 0 / 0 at d39 (gain 0) | does not qualify at both references |
| 6 | R3710 / 34 | 17-13 | 17-13 / 0 / d30 | - | - | - | 20M chooses played move |
| 7 | R6637 / 38 | 30-25 | 30-25 / 0 / d27 | - | - | - | 20M chooses played move |
| 8 | R7895 / 36 | 27x18 | 27x18 / -90 / d1 | - | - | - | only legal move |
| 9 | R3312 / 48 | 3-8 | 3-8 / 0 / d34 | - | - | - | 20M chooses played move |
| 10 | R6368 / 32 | 21-17 | 21-17 / 0 / d39 | - | - | - | 20M chooses played move |
| 11 | R513 / 31 | 19-23 | 19-23 / 0 / d28 | - | - | - | 20M chooses played move |
| 12 | R1855 / 41 | 23-27 | 3-7 / 0 / d28 | 3-7 / 0 / d33 | 0 / 0 at d28 (gain 0) | 0 / 0 at d33 (gain 0) | does not qualify at both references |
| 13 | R4515 / 39 | 12-16 | 26-22 / 0 / d34 | 26-22 / 0 / d40 | 0 / 0 at d34 (gain 0) | 0 / 0 at d40 (gain 0) | does not qualify at both references |
| 14 | R1336 / 51 | 15-18 | 16-20 / 0 / d30 | 16-20 / 0 / d36 | 0 / 0 at d30 (gain 0) | 0 / 0 at d35 (gain 0) | does not qualify at both references |
| 15 | R1713 / 24 | 27-24 | 15-10 / 5 / d27 | 15-10 / 0 / d32 | 0 / 0 at d27 (gain 0) | 0 / 0 at d32 (gain 0) | does not qualify at both references |
| 16 | R3319 / 37 | 7-11 | 7-11 / 0 / d30 | - | - | - | 20M chooses played move |
| 17 | R5087 / 43 | 24-27 | 24-27 / 0 / d52 | - | - | - | 20M chooses played move |
| 18 | R5403 / 43 | 24-27 | 24-27 / 0 / d52 | - | - | - | 20M chooses played move |
| 19 | R6649 / 43 | 24-27 | 24-27 / 0 / d52 | - | - | - | 20M chooses played move |
| 20 | R7614 / 47 | 7-11 | 7-10 / 0 / d32 | 7-10 / 0 / d37 | 0 / 0 at d32 (gain 0) | 0 / 0 at d37 (gain 0) | does not qualify at both references |

### Frequencies for every checked predecessor

These counts are supplied whether or not the move qualifies as an error. They use the named opponent and mover role in the entire reviewed pool, with full-board transpositions counted. Counts are distinct games; repeated visits are also retained in [predecessor-census.tsv](predecessor-census.tsv). Outcomes inside the choice summary are from Regicide’s side.

| Rank | Game / ply | Opening | Ours | Opponent | FEN | Played | Games reaching | Games playing it | Every observed choice |
|---|---|---|---|---|---|---|---|---|---|
| 1 | R4102 / 104 | ACF #16 Wilderness II | B | kingsrow | W:WK3,K10,19,20,K24,28:BK2,4,K5,12,K17,K30 | 3-7 | 1 | 1 | 3-7: 1 games / 1 visits (+0 =1 -0) |
| 2 | R2794 / 45 | ACF #162 | W | kingsrow | B:W14,15,22,25,27:B5,12,13,18,20 | 13-17 | 1 | 1 | 13-17: 1 games / 1 visits (+0 =1 -0) |
| 3 | R4329 / 29 | ACF #150 | W | kingsrow | B:W9,18,20,21,26,30,31,32:B1,2,3,7,12,13,19,24 | 12-16 | 1 | 1 | 12-16: 1 games / 1 visits (+0 =1 -0) |
| 4 | R1762 / 31 | ACF #110 | W | cake | B:W15,16,17,20,21,26,28,30,31:B2,4,6,7,9,10,12,19,25 | 19-23 | 12 | 5 | 19-23: 5 games / 5 visits (+0 =5 -0); 9-13: 7 games / 7 visits (+0 =7 -0) |
| 5 | R946 / 56 | ACF #5 Dreaded Edinburgh | B | kingsrow | W:WK10,11,14,21,23,28:B5,12,13,16,20,K31 | 23-19 | 1 | 1 | 23-19: 1 games / 1 visits (+0 =1 -0) |
| 6 | R3710 / 34 | ACF #152 | B | cake | W:W6,17,21,23,29,31,32:B1,2,8,10,12,14,24,28 | 17-13 | 1 | 1 | 17-13: 1 games / 1 visits (+0 =1 -0) |
| 7 | R6637 / 38 | ACF #138 | B | cake | W:W5,10,13,14,21,27,30,31,32:B1,2,6,12,16,18,19,20,24 | 30-25 | 1 | 1 | 30-25: 1 games / 1 visits (+0 =1 -0) |
| 8 | R7895 / 36 | ACF #148 | B | kingsrow | W:W11,13,17,21,26,27,31:B2,4,5,6,15,19,23 | 27x18 | 1 | 1 | 27x18: 1 games / 1 visits (+0 =1 -0) |
| 9 | R3312 / 48 | ACF #102 | B | cake | W:WK3,13,17,20,21,23,32:B5,6,9,10,15,K26,28 | 3-8 | 1 | 1 | 3-8: 1 games / 1 visits (+0 =1 -0) |
| 10 | R6368 / 32 | ACF #169 | B | kingsrow | W:W20,21,25,26,28,30,32:B4,8,10,11,14,15,18,19 | 21-17 | 1 | 1 | 21-17: 1 games / 1 visits (+0 =1 -0) |
| 11 | R513 / 31 | ACF #110 | W | cake | B:W15,16,17,20,21,26,28,30,31:B2,4,6,7,9,10,12,19,25 | 19-23 | 12 | 5 | 19-23: 5 games / 5 visits (+0 =5 -0); 9-13: 7 games / 7 visits (+0 =7 -0) |
| 12 | R1855 / 41 | ACF #160 | W | cake | B:WK1,12,14,16,29,30,31:B2,3,5,13,18,22,23 | 23-27 | 1 | 1 | 23-27: 1 games / 1 visits (+0 =1 -0) |
| 13 | R4515 / 39 | ACF #80 | W | kingsrow | B:W5,18,19,20,25,27,28:B1,3,10,11,12,K26 | 12-16 | 1 | 1 | 12-16: 1 games / 1 visits (+0 =1 -0) |
| 14 | R1336 / 51 | ACF #49 | W | kingsrow | B:WK6,10,21,22,30:B3,4,15,16,K23 | 15-18 | 1 | 1 | 15-18: 1 games / 1 visits (+0 =1 -0) |
| 15 | R1713 / 24 | ACF #87 Tyne | B | cake | W:W14,15,18,21,22,26,27,28,30,31:B1,2,3,5,7,9,12,13,19,20 | 27-24 | 4 | 3 | 15-10: 1 games / 1 visits (+0 =1 -0); 27-24: 3 games / 3 visits (+0 =3 -0) |
| 16 | R3319 / 37 | ACF #110 | W | cake | B:W13,15,16,19,20,21,26,28,31:B2,6,7,8,9,10,12,K29 | 7-11 | 1 | 1 | 7-11: 1 games / 1 visits (+0 =1 -0) |
| 17 | R5087 / 43 | ACF #147 | W | cake | B:W13,17,18,21,32:B5,10,11,16,24 | 24-27 | 3 | 3 | 24-27: 3 games / 3 visits (+0 =3 -0) |
| 18 | R5403 / 43 | ACF #147 | W | cake | B:W13,17,18,21,32:B5,10,11,16,24 | 24-27 | 3 | 3 | 24-27: 3 games / 3 visits (+0 =3 -0) |
| 19 | R6649 / 43 | ACF #147 | W | cake | B:W13,17,18,21,32:B5,10,11,16,24 | 24-27 | 3 | 3 | 24-27: 3 games / 3 visits (+0 =3 -0) |
| 20 | R7614 / 47 | ACF #157 | W | kingsrow | B:WK3,16,24,25,27,28:B7,13,15,18,20,26 | 7-11 | 1 | 1 | 7-11: 1 games / 1 visits (+0 =1 -0) |

## Other mining-list contrasts

The 36 inputs contain the already-confirmed ACF 71 point and 35 GuiNN points. Their original result contrast crosses nets/books, so it is descriptive evidence rather than a controlled estimate of move quality. The largest point is closed by identical-board/history round-1 reuse; all other completed points below are newly searched.

| Rank | Node | Opponent | Opening | Ours | Ply | Losing-game move | Alternative | 20M pair | 100M pair | Qualified | Roots reject played |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 881.24 | kingsrow | ACF #71 Diabolical Denny | W | 25 | 10-14 | 5-9 | -188 / -3 at d27 (gain 185) | -186 / 0 at d32 (gain 186) | True | True |
| 2 | 315.6 | guinn | ACF #10 | W | 7 | 10-15 | 16-20 | -40 / -6 at d25 (gain 34) | -18 / 0 at d29 (gain 18) | False | True |
| 3 | 319.6 | guinn | ACF #101 | W | 7 | 9-14 | 6-10 | -22 / 0 at d24 (gain 22) | -8 / 0 at d29 (gain 8) | False | True |
| 4 | 338.3 | guinn | ACF #111 | B | 4 | 25-21 | 17-13 | -45 / 14 at d25 (gain 59) | -13 / 0 at d29 (gain 13) | False | True |
| 5 | 346.3 | guinn | ACF #116 | B | 4 | 25-22 | 17-14 | -25 / -7 at d25 (gain 18) | -11 / 0 at d29 (gain 11) | False | True |
| 6 | 349.6 | guinn | ACF #117 | W | 7 | 15-18 | 8-11 | -24 / 0 at d25 (gain 24) | -44 / 0 at d29 (gain 44) | False | True |
| 7 | 354.3 | guinn | ACF #12 | B | 4 | 24-20 | 26-23 | 0 / 29 at d25 (gain 29) | 0 / 10 at d29 (gain 10) | False | True |
| 7 | 354.3 | guinn | ACF #12 | B | 4 | 24-20 | 27-23 | 0 / 8 at d25 (gain 8) | 0 / 0 at d29 (gain 0) | False | True |
| 8 | 365.6 | guinn | ACF #124 | W | 7 | 9-14 | 7-11 | -26 / -27 at d25 (gain -1) | -13 / -4 at d28 (gain 9) | False | True |
| 9 | 374.23 | guinn | ACF #129 | B | 24 | 27-24 | 31-26 | -11 / 0 at d26 (gain 11) | 0 / 0 at d31 (gain 0) | False | True |
| 10 | 385.14 | guinn | ACF #134 | W | 15 | 14-17 | 7-10 | -24 / -19 at d26 (gain 5) | -20 / -17 at d29 (gain 3) | False | False |
| 10 | 385.14 | guinn | ACF #134 | W | 15 | 7-10 | 14-17 | -19 / -24 at d26 (gain -5) | -17 / -20 at d29 (gain -3) | False | True |
| 11 | 397.10 | guinn | ACF #140 | W | 11 | 11-16 | 12-16 | -4 / -11 at d24 (gain -7) | -35 / -10 at d30 (gain 25) | False | True |
| 12 | 399.12 | guinn | ACF #141 | W | 13 | 16-20 | 11-15 | -189 / -72 at d24 (gain 117) | -189 / -56 at d25 (gain 133) | True | True |
| 12 | 399.12 | guinn | ACF #141 | W | 13 | 16-20 | 4-8 | -189 / -167 at d24 (gain 22) | -189 / -161 at d25 (gain 28) | False | True |
| 12 | 399.12 | guinn | ACF #141 | W | 13 | 4-8 | 11-15 | -167 / -72 at d24 (gain 95) | -167 / -37 at d27 (gain 130) | True | True |
| 12 | 399.12 | guinn | ACF #141 | W | 13 | 4-8 | 16-20 | -167 / -189 at d24 (gain -22) | -161 / -189 at d25 (gain -28) | False | True |

Updated board frequencies for every completed mining entry are in [mining-census.tsv](mining-census.tsv); they are separate from the older input contrast. The full ordered backlog is in mining-coverage.tsv. Input copies are input-mining-candidates.tsv and input-mining-evidence.tsv; all measured fields and root choices are in mining-results.tsv.

## Database endpoints

The cutoff is the first recorded board with <=8 pieces, on either side’s turn. It is separately audited even when the preceding walk is not yet complete. If it is our turn, that position is also included in the own-choice check. At first material entry the reversible count resets; the old game history cannot repeat this lower-material board. All 112 probes return WLD Draw (2). WLD codes in the evidence are 0 Unknown, 1 Win, 2 Draw, 3 Loss, always from the side to move; child values therefore use the next player’s view. These WLD facts are not a promise of conversion before a no-progress limit.

| Game | Opening | Ours | Cutoff ply | Root DB | Played | Child DB |
|---|---|---|---|---|---|---|
| R1005 | ACF #16 Wilderness II | W | 203 | 2 | 19-24 | 2 |
| R101 | ACF #58 | W | 52 | 2 | 32-27 | 2 |
| R1058 | ACF #69 | B | 55 | 2 | 11-7 | 2 |
| R1089 | ACF #84 | B | 55 | 2 | 29-25 | 2 |
| R1106 | ACF #97 | W | 42 | 2 | 15-10 | 2 |
| R1216 | ACF #153 Black Widow | W | 59 | 2 | 31x22 | 2 |
| R1327 | ACF #47 | W | 44 | 2 | 20-16 | 2 |
| R1336 | ACF #49 | W | 58 | 2 | 9-5 | 2 |
| R134 | ACF #72 | B | 60 | 2 | 13-9 | 2 |
| R1384 | ACF #74 | B | 55 | 2 | 26-30 | 2 |
| R1524 | ACF #150 | W | 52 | 2 | 27-24 | 2 |
| R154 | ACF #87 Tyne | B | 46 | 2 | 22-18 | 2 |
| R1693 | ACF #75 | W | 37 | 2 | 1-6 | 2 |
| R1713 | ACF #87 Tyne | B | 46 | 2 | 22-18 | 2 |
| R1762 | ACF #110 | W | 53 | 2 | 9-14 | 2 |
| R1801 | ACF #132 | W | 49 | 2 | 22x13 | 2 |
| R1824 | ACF #141 | W | 91 | 2 | 27-32 | 2 |
| R1852 | ACF #163 | W | 52 | 2 | 15-10 | 2 |
| R1855 | ACF #160 | W | 63 | 2 | 23-27 | 2 |
| R1859 | ACF #154 | B | 117 | 2 | 26-30 | 2 |
| R1897 | ACF #15 | W | 57 | 2 | 22-26 | 2 |
| R1903 | ACF #19 | W | 42 | 2 | 17-13 | 2 |
| R1944 | ACF #33 | W | 85 | 2 | 25-30 | 2 |
| R2080 | ACF #112 | W | 54 | 2 | 7-2 | 2 |
| R2136 | ACF #147 | B | 54 | 2 | 1-6 | 2 |
| R2172 | ACF #163 | W | 62 | 2 | 1-6 | 2 |
| R245 | ACF #139 | B | 33 | 2 | 7-10 | 2 |
| R2519 | ACF #9 Wilderness I | W | 82 | 2 | 14-9 | 2 |
| R2551 | ACF #16 Wilderness II | W | 132 | 2 | 14-9 | 2 |
| R2616 | ACF #70 | B | 43 | 2 | 23-26 | 2 |
| R2660 | ACF #92 | B | 48 | 2 | 6x15 | 2 |
| R2744 | ACF #134 | W | 48 | 2 | 30-25 | 2 |
| R2750 | ACF #139 | W | 64 | 2 | 22-17 | 2 |
| R2794 | ACF #162 | W | 52 | 2 | 15-10 | 2 |
| R285 | ACF #160 | W | 35 | 2 | 9-14 | 2 |
| R2972 | ACF #92 | B | 48 | 2 | 6x15 | 2 |
| R3023 | ACF #120 | W | 44 | 2 | 27-23 | 2 |
| R3156 | ACF #9 Wilderness I | B | 119 | 2 | 25-21 | 2 |
| R3179 | ACF #31 | W | 54 | 2 | 24-19 | 2 |
| R3194 | ACF #43 | B | 44 | 2 | 17-14 | 2 |
| R3214 | ACF #50 | W | 64 | 2 | 14-9 | 2 |
| R3293 | ACF #99 | B | 42 | 2 | 6-2 | 2 |
| R3312 | ACF #102 | B | 68 | 2 | 1-6 | 2 |
| R3319 | ACF #110 | W | 53 | 2 | 9-14 | 2 |
| R3340 | ACF #122 | W | 42 | 2 | 21-17 | 2 |
| R3363 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R3373 | ACF #142 | W | 48 | 2 | 24-19 | 2 |
| R3507 | ACF #43 | B | 44 | 2 | 17-14 | 2 |
| R3604 | ACF #99 | B | 42 | 2 | 6-2 | 2 |
| R3640 | ACF #117 | W | 40 | 2 | 31x22x13 | 2 |
| R3672 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R3686 | ACF #142 | W | 43 | 2 | 9-14 | 2 |
| R3710 | ACF #152 | B | 53 | 2 | 27-24 | 2 |
| R382 | ACF #33 | W | 67 | 2 | 32-28 | 2 |
| R396 | ACF #50 | W | 44 | 2 | 26-22 | 2 |
| R4095 | ACF #16 Wilderness II | W | 89 | 2 | 20-24 | 2 |
| R4102 | ACF #16 Wilderness II | B | 111 | 2 | 30-25 | 2 |
| R4116 | ACF #36 | W | 46 | 2 | 17-13 | 2 |
| R4329 | ACF #150 | W | 54 | 2 | 11-7 | 2 |
| R4515 | ACF #80 | W | 53 | 2 | 3x10x17 | 2 |
| R456 | ACF #82 | B | 41 | 2 | 2-6 | 2 |
| R457 | ACF #82 | W | 42 | 2 | 23-19 | 2 |
| R4581 | ACF #115 | W | 51 | 2 | 26-31 | 2 |
| R4621 | ACF #142 | W | 48 | 2 | 30-25 | 2 |
| R4831 | ACF #1 | W | 49 | 2 | 26-31 | 2 |
| R4999 | ACF #93 | W | 47 | 2 | 25-30 | 2 |
| R5072 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R5087 | ACF #147 | W | 50 | 2 | 17x10 | 2 |
| R5121 | ACF #160 | W | 65 | 2 | 23-18 | 2 |
| R513 | ACF #110 | W | 53 | 2 | 9-14 | 2 |
| R5131 | ACF #173 | B | 45 | 2 | 11-16 | 2 |
| R5143 | ACF #1 | W | 49 | 2 | 26-31 | 2 |
| R5191 | ACF #31 | W | 50 | 2 | 24-19 | 2 |
| R523 | ACF #118 | B | 40 | 2 | 14-10 | 2 |
| R5248 | ACF #55 | W | 81 | 2 | 32-28 | 2 |
| R527 | ACF #118 | W | 49 | 2 | 15-19 | 2 |
| R5309 | ACF #93 | W | 47 | 2 | 25-30 | 2 |
| R533 | ACF #125 | W | 32 | 2 | 27-23 | 2 |
| R5378 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R540 | ACF #128 | W | 47 | 2 | 1x10 | 2 |
| R5403 | ACF #147 | W | 50 | 2 | 17x10 | 2 |
| R566 | ACF #142 | B | 43 | 2 | 24-27 | 2 |
| R5836 | ACF #16 Wilderness II | W | 128 | 2 | 11-7 | 2 |
| R596 | ACF #160 | W | 35 | 2 | 9-14 | 2 |
| R5975 | ACF #121 | W | 38 | 2 | 25-21 | 2 |
| R6015 | ACF #134 | W | 58 | 2 | 6-1 | 2 |
| R6143 | ACF #36 | W | 46 | 2 | 17-13 | 2 |
| R6171 | ACF #55 | B | 59 | 2 | 26-22 | 2 |
| R6326 | ACF #134 | W | 58 | 2 | 6-1 | 2 |
| R6332 | ACF #142 | W | 53 | 2 | 26-22 | 2 |
| R6368 | ACF #169 | B | 41 | 2 | 18-22 | 2 |
| R6625 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R6637 | ACF #138 | B | 69 | 2 | 19-15 | 2 |
| R6649 | ACF #147 | W | 50 | 2 | 17x10 | 2 |
| R6795 | ACF #55 | W | 64 | 2 | 24-19 | 2 |
| R6898 | ACF #116 | B | 34 | 2 | 29-25 | 2 |
| R6938 | ACF #134 | W | 36 | 2 | 24-19 | 2 |
| R6986 | ACF #143 | B | 146 | 2 | 11-7 | 2 |
| R7346 | ACF #15 | B | 58 | 2 | 15x22 | 2 |
| R7360 | ACF #17 | B | 82 | 2 | 26x19x12 | 2 |
| R7398 | ACF #16 Wilderness II | W | 216 | 2 | 6-1 | 2 |
| R7512 | ACF #107 | B | 41 | 2 | 7-10 | 2 |
| R7577 | ACF #139 | W | 72 | 2 | 20-16 | 2 |
| R7601 | ACF #157 | B | 37 | 2 | 16-19 | 2 |
| R7614 | ACF #157 | W | 60 | 2 | 28-24 | 2 |
| R7683 | ACF #30 | B | 45 | 2 | 3x12 | 2 |
| R7822 | ACF #107 | B | 41 | 2 | 5-9 | 2 |
| R7852 | ACF #115 | W | 70 | 2 | 10-6 | 2 |
| R7895 | ACF #148 | B | 50 | 2 | 17x10x1 | 2 |
| R83 | ACF #50 | W | 44 | 2 | 26-22 | 2 |
| R93 | ACF #54 | W | 51 | 2 | 20-16 | 2 |
| R946 | ACF #5 Dreaded Edinburgh | B | 63 | 2 | 31-26 | 2 |

Selected games without an eight-piece endpoint: R1073 (wp11c/games/bl-v11c-n6c-kingsrow-a-h1.pdn.gz 66.1), R1330 (wp11c/games/bl-v11c-n6c-kingsrow-b-h1.pdn.gz 14.2), R7679 (tests/v11d-n8q-kingsrow-b-h1/games.pdn.gz 14.2), R3542 (wp11d/games/bl-v11d-n6c-cake-b-h1.pdn.gz 47.2).

## What ran and what was only read

Ran: full legal replay of the committed corpus; selection from actual comments; sequential current-engine root/restricted queries in the completed scope; direct endpoint WLD probes; and node/depth/result validation of every published trace. Read: the current master ledger, the original mining evidence, and the source configuration metadata. The four ACF 71 traces are reused measurements, explicitly marked in their headers. No new matches, broad ctest run, sanitizer pass, engine code change, training-data write or database modification was performed. Partial/unprocessed games and mining entries are listed, not inferred from the completed subset.

Only report text is committed under reviews/b/round2, after fetching and rebasing on the shared reports branch. Each artifact is UTF-8 text under 1 MB. The query source, selection procedure and comparison driver follow as report text so the measurements can be reproduced without access to the local machine.

## Reproduction sources

### prepare.py

```python
import csv,gzip,json,re,sys,collections
from pathlib import Path
sys.path.insert(0,'snapshots/booklab/tools/booklab')
from theory import parse_pdn,ballot_of,family,score_of,is_book
base=Path('snapshots/booklab/experiments/booklab')
files=sorted(set(list((base/'wp11c/games').rglob('*.pdn*'))+list((base/'wp11d/games').rglob('*.pdn*'))+list((base/'mo').rglob('*.pdn*'))))
games=[];selected=[]
for p in files:
 if not p.name.endswith(('.pdn','.pdn.gz')):continue
 text=gzip.open(p,'rt',errors='replace').read() if p.suffix=='.gz' else p.read_text(errors='replace')
 for t,mc in parse_pdn(text):
  rb=t.get('Black','').lower().startswith('regicide');rw=t.get('White','').lower().startswith('regicide')
  if rb==rw:continue
  col='B' if rb else 'W';opp=family(t.get('White' if rb else 'Black',''))
  g=dict(id=f'R{len(games)}',source=str(p.relative_to(base)),run=p.name.removesuffix('.gz').removesuffix('.pdn'),rnd=t.get('Round',''),fen=t.get('FEN',''),ours=col,opp=opp,opening=ballot_of(t.get('Opening',''))[0],nopen=ballot_of(t.get('Opening',''))[2],result=t.get('Result',''),moves=[m for m,c in mc],comments=[c for m,c in mc],tags=t)
  games.append(g)
  if opp not in ('cake','kingsrow') or g['result']!='1/2-1/2':continue
  own=[]
  for i,(m,c) in enumerate(mc):
   mover= ('B' if i%2==0 else 'W') if not g['fen'] else (g['fen'][0] if i%2==0 else ('W' if g['fen'][0]=='B' else 'B'))
   if mover!=col or i<g['nopen'] or is_book('regicide',c):continue
   sc=score_of(c)
   if sc is not None:own.append((i+1,sc))
  highs=[(p,s) for p,s in own if s>=40]
  if highs:
   g['first40']=highs[0][0];g['first40score']=highs[0][1];g['peak']=max(s for p,s in own);selected.append(g['id'])
Path('work/round2/games.json').write_text(json.dumps(games,indent=2))
with open('work/round2/games.tsv','w') as f:
 for g in games:f.write('\t'.join([g['id'],g['fen'],g['ours'],str(g['nopen']),' '.join(g['moves'])])+'\n')
selected.sort(key=lambda k:(-games[int(k[1:])]['peak'],games[int(k[1:])]['source'],games[int(k[1:])]['rnd']))
Path('work/round2/selected.json').write_text(json.dumps(selected,indent=2))
with open('reports/round2/selected-games.tsv','w') as f:
 w=csv.writer(f,delimiter='\t');w.writerow(['rank','id','source','round','opening','ours','opponent','first40ply','first40score','peak'])
 for rank,gid in enumerate(selected,1):
  g=games[int(gid[1:])];w.writerow([rank,gid,g['source'],g['rnd'],g['opening'],g['ours'],g['opp'],g['first40'],g['first40score'],g['peak']])
print('PDN files',len(files),'games',len(games),'selected drawn games',len(selected))
print('selection by opponent',dict(collections.Counter(games[int(k[1:])]['opp'] for k in selected)))
for rank,k in enumerate(selected[:20],1):
 g=games[int(k[1:])];print(rank,k,g['run'],g['rnd'],g['opening'],g['ours'],'first',g['first40'],g['first40score'],'peak',g['peak'])
mines=list(csv.DictReader(open('work/round2/mining-candidates-001.tsv'),delimiter='\t'))
print('Mining candidates',len(mines),'not within-conf outcome contrast',sum(r['within_configuration_outcome_difference']=='0' for r in mines))
```

### append_inputs.py

```python
import csv,gzip,json,re,sys,collections
from pathlib import Path
sys.path.insert(0,'snapshots/booklab/tools/booklab')
from theory import parse_pdn,ballot_of,family,score_of,is_book
P=Path('work/round2');base=Path('snapshots/booklab/experiments/booklab')
games=json.load(open(P/'games.json'));known={(g['run'].removeprefix('bl-'),g['rnd'],g['ours']):g for g in games}
files=sorted(set(list((base/'mo').rglob('*.pdn*'))+list((base/'tests').glob('mo-*/games.pdn.gz'))+list((base/'tests').glob('v11d-n8q-*/games.pdn.gz'))))
added=dups=0
for p in files:
 if not p.name.endswith(('.pdn','.pdn.gz')):continue
 text=gzip.open(p,'rt',errors='replace').read() if p.suffix=='.gz' else p.read_text(errors='replace')
 run=p.parent.name if p.name.startswith('games.pdn') else p.name.removesuffix('.gz').removesuffix('.pdn')
 if run.startswith('v11d-'):run='bl-'+run
 for t,mc in parse_pdn(text):
  rb=t.get('Black','').lower().startswith('regicide');rw=t.get('White','').lower().startswith('regicide')
  if rb==rw:continue
  col='B' if rb else 'W';opp=family(t.get('White' if rb else 'Black',''));key=(run.removeprefix('bl-'),t.get('Round',''),col)
  if key in known:
   old=known[key]
   assert old['moves']==[m for m,c in mc] and old['result']==t.get('Result','') and old['fen']==t.get('FEN',''),key
   aliases=old.setdefault('aliases',[])
   if str(p.relative_to(base)) not in aliases:aliases.append(str(p.relative_to(base)))
   dups+=1;continue
  g=dict(id=f'R{len(games)}',source=str(p.relative_to(base)),run=run,rnd=t.get('Round',''),fen=t.get('FEN',''),ours=col,opp=opp,opening=ballot_of(t.get('Opening',''))[0],nopen=ballot_of(t.get('Opening',''))[2],result=t.get('Result',''),moves=[m for m,c in mc],comments=[c for m,c in mc],tags=t)
  games.append(g);known[key]=g;added+=1
# Recompute triggers from actual search comments, retaining original game IDs.
selected=[]
for g in games:
 for k in ('first40','first40score','peak'):g.pop(k,None)
 if g['opp'] not in ('cake','kingsrow') or g['result']!='1/2-1/2':continue
 own=[];initial=g['fen'][0] if g['fen'] else 'B'
 for i,c in enumerate(g['comments']):
  stm=initial if i%2==0 else ('W' if initial=='B' else 'B')
  if stm!=g['ours'] or i<g['nopen'] or is_book('regicide',c):continue
  sc=score_of(c)
  if sc is not None:own.append((i+1,sc))
 high=[(p,s) for p,s in own if s>=40]
 if high:g.update(first40=high[0][0],first40score=high[0][1],peak=max(s for p,s in own));selected.append(g['id'])
gs={g['id']:g for g in games};selected.sort(key=lambda k:(-gs[k]['peak'],gs[k]['source'],gs[k]['rnd']))
P.joinpath('games.json').write_text(json.dumps(games,indent=2));P.joinpath('selected.json').write_text(json.dumps(selected,indent=2))
with open(P/'games.tsv','w') as f:
 for g in games:f.write('\t'.join([g['id'],g['fen'],g['ours'],str(g['nopen']),' '.join(g['moves'])])+'\n')
with open('reports/round2/selected-games.tsv','w') as f:
 w=csv.writer(f,delimiter='\t');w.writerow(['rank','id','source','round','opening','ours','opponent','first40ply','first40score','peak'])
 for rank,k in enumerate(selected,1):
  g=gs[k];w.writerow([rank,k,g['source'],g['rnd'],g['opening'],g['ours'],g['opp'],g['first40'],g['first40score'],g['peak']])
print('Added',added,'archived duplicate identities',dups,'corpus',len(games),'selected',len(selected))
print('By opponent',dict(collections.Counter(gs[k]['opp'] for k in selected)))
for n,k in enumerate(selected[:24],1):
 g=gs[k];print(n,k,g['run'],g['rnd'],g['opening'],g['ours'],'first',g['first40'],g['first40score'],'peak',g['peak'])
```

### make_plan.py

```python
import csv,json,collections
from pathlib import Path
P=Path('work/round2');gs={g['id']:g for g in json.load(open(P/'games.json'))};sel=json.load(open(P/'selected.json'));needed=set(sel);rs=collections.defaultdict(list)
for r in csv.DictReader(open(P/'walk.tsv'),delimiter='\t'):
 if r['game'] in needed:rs[r['game']].append(r)
plan=[]
for rank,k in enumerate(sel,1):
 g=gs[k];ar=[r for r in rs[k] if int(r['ply'])>=g['first40']]
 cut=next((int(r['ply']) for r in ar if int(r['pieces'])<=8),len(g['moves'])+1)
 own=[r for r in ar if int(r['ply'])<cut and r['fen'][0]==g['ours']]
 plan.append(dict(rank=rank,game=k,first=g['first40'],peak=g['peak'],cutoff=cut,own_moves=len(own),search_roots=sum(int(r['legal'])>1 for r in own)))
P.joinpath('walk-plan.json').write_text(json.dumps(plan,indent=2))
```

### contexts.py

```python
import csv,json,collections
from pathlib import Path
P=Path('work/round2');gs={g['id']:g for g in json.load(open(P/'games.json'))};sel=json.load(open(P/'selected.json'));plan=json.load(open(P/'walk-plan.json'))
mines=[r for r in csv.DictReader(open(P/'mining-candidates-001.tsv'),delimiter='\t') if r['within_configuration_outcome_difference']=='0']
mines.sort(key=lambda r:(-int(r['games']),tuple(map(int,r['node'].split('.')))))
evidence=list(csv.DictReader(open(P/'mining-evidence.tsv'),delimiter='\t'))
byrecord={(g['run'],g['rnd']):g['id'] for g in gs.values()}
targets=collections.defaultdict(dict);ownjobs=[];prevjobs=[];minejobs=[]
for q in plan:
 g=gs[q['game']]
 for ply in range(q['first'],q['cutoff']):
  # Actual own colour, with full FEN checked below.
  if (ply%2==1)==(g['ours']=='B'):
   targets[g['id']][ply]=True;ownjobs.append(dict(rank=q['rank'],game=g['id'],ply=ply))
 if q['cutoff']<=len(g['moves']) and ((q['cutoff']%2==1)==(g['ours']=='B')):
  targets[g['id']][q['cutoff']]=True;ownjobs.append(dict(rank=q['rank'],game=g['id'],ply=q['cutoff'],endpoint=True))
 if q['first']>1:
  targets[g['id']][q['first']-1]=True;prevjobs.append(dict(rank=q['rank'],game=g['id'],ply=q['first']-1))
for rank,m in enumerate(mines,1):
 ev=[r for r in evidence if r['node']==m['node']];wins=[r for r in ev if r['result']=='W'];draws=[r for r in ev if r['result']=='D']
 assert wins and draws,m
 ev=sorted(ev,key=lambda r:(r['result']!='W',r['run'],r['round']))
 r=ev[0];gid=byrecord[r['run'],r['round']];ply=int(m['ply']);targets[gid][ply]=True
 minejobs.append(dict(rank=rank,game=gid,ply=ply,node=m['node'],fen=m['fen'],losing_moves=sorted({r['move'] for r in wins}),drawing_moves=sorted({r['move'] for r in draws}),evidence=ev,meta=m))
contexts=json.loads((P/'contexts.json').read_text()) if (P/'contexts.json').exists() else []
keyids={'\n'.join([c['fen'],str(c['rev'])]+c['trail']):c['id'] for c in contexts}
at={};hist=[];prev=None
for r in csv.DictReader(open(P/'walk.tsv'),delimiter='\t'):
 gid=r['game'];ply=int(r['ply'])
 if gid!=prev:hist=[];prev=gid
 if ply in targets.get(gid,{}):
  rev=int(r['rev']);trail=hist[-rev:] if rev else []
  sig='\n'.join([r['fen'],str(rev)]+trail)
  if sig not in keyids:
   cid=f'C{len(contexts):04}';keyids[sig]=cid;contexts.append(dict(id=cid,game=gid,ply=ply,fen=r['fen'],rev=rev,pieces=int(r['pieces']),legal=int(r['legal']),played=r['played'],trail=trail))
  at[gid,ply]=keyids[sig]
 hist.append(r['fen'])
for items in (ownjobs,prevjobs,minejobs):
 for q in items:
  q['context']=at[q['game'],q['ply']]
  c=contexts[int(q['context'][1:])]
  assert c['fen'][0]==(gs[q['game']]['ours'] if items is ownjobs else ('W' if gs[q['game']]['ours']=='B' else 'B'))
  if items is minejobs:assert c['fen']==q['fen'],q
# Own job played move can differ across repeated shared contexts.
indexed={}
for r in csv.DictReader(open(P/'walk.tsv'),delimiter='\t'):
 key=(r['game'],int(r['ply']))
 if key in at:indexed[key]=r
for items in (ownjobs,prevjobs,minejobs):
 for q in items:q['played']=indexed[q['game'],q['ply']]['played']
P.joinpath('contexts.json').write_text(json.dumps(contexts,indent=2))
P.joinpath('jobs.json').write_text(json.dumps(dict(own=ownjobs,previous=prevjobs,mining=minejobs),indent=2))
print('unique requested contexts',len(contexts),'own jobs',len(ownjobs),'opponent-before jobs',len(prevjobs),'mining',len(minejobs))
print('first 12 games own positions',sum(q['rank']<=12 for q in ownjobs),'unique',len({q['context'] for q in ownjobs if q['rank']<=12}))
# Full board census, recording mover role; all nets, results and opponents in the committed corpus.
fens={c['fen'] for c in contexts};occ=[]
for r in csv.DictReader(open(P/'walk.tsv'),delimiter='\t'):
 if r['fen'] not in fens:continue
 g=gs[r['game']];occ.append(dict(game=g['id'],run=g['run'],round=g['rnd'],source=g['source'],opening=g['opening'],ours=g['ours'],opponent=g['opp'],result=g['result'],ply=r['ply'],fen=r['fen'],rev=r['rev'],move=r['played'],role='own' if r['fen'][0]==g['ours'] else 'opponent'))
P.joinpath('occurrences.json').write_text(json.dumps(occ))
with open('reports/round2/mining-order.tsv','w') as f:
 w=csv.writer(f,delimiter='\t');w.writerow(['rank','node','context','opponent','opening','colour','ply','games','losing_game_moves','drawn_game_moves'])
 for q in minejobs:w.writerow([q['rank'],q['node'],q['context'],q['meta']['opponent'],q['meta']['opening'],q['meta']['our_colour'],q['ply'],q['meta']['games'],','.join(q['losing_moves']),','.join(q['drawing_moves'])])
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
P=Path('work/round2');D=Path('reports/round2/queries')
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
 with p.open('w') as f:
  subprocess.run(['nice','-n','19','work/round2/replay','query','work/round2/games.tsv',c['game'],str(c['ply']),str(b),str(depth),move,'8'],stdout=f,check=True,timeout=820)
 r=read(p);return r
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
def save():statepath.write_text(json.dumps(state,indent=2))
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

### dbinspect.cpp

```cpp
#include <fstream>
#include <sstream>
#include <iostream>
#include "position.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
using namespace regicide;
int main(int ac,char**av){if(ac!=2||!egdb::init("/home/admingod/regicide-data/egdb",64))return 2;std::ifstream in(av[1]);std::cout<<"game\tply\tfen\tours\tplayed\troot_db\tplayed_child_db\tchildren\n";
for(std::string s;std::getline(in,s);){std::istringstream ss(s);std::string id,ps,fen,ours,played;std::getline(ss,id,'\t');std::getline(ss,ps,'\t');std::getline(ss,fen,'\t');std::getline(ss,ours,'\t');std::getline(ss,played,'\t');Position p;if(!p.set_fen(fen)||p.piece_count()>8)return 3;auto val=[](const Position&q){return int(egdb::probe(q.pieces(BLACK),q.pieces(WHITE),q.kings(),q.side_to_move()));};Move actual;if(!parse_move(p,played,actual))return 4;Position a=p;a.make_move(actual);std::cout<<id<<'\t'<<ps<<'\t'<<fen<<'\t'<<ours<<'\t'<<move_text(p,actual)<<'\t'<<val(p)<<'\t'<<val(a)<<'\t';MoveList ml;generate_moves(p,ml);for(auto&m:ml){Position c=p;c.make_move(m);std::cout<<move_text(p,m)<<':'<<val(c)<<',';}std::cout<<'\n';}return 0;}
```
