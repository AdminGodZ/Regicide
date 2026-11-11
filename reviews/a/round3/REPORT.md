# Review A, round 3: measured variation and pair scheduling

## Results and evidence cutoff

No scheduling defect was found in PR #73 at master **1c6e99715299a5fbbbd407b1b9361e3c299e91e6**. The proposed asymmetric-restart regression below passes master and fails a mutation restoring the shared-counter reset.

The move map covers the available committed records at Book Lab **33e89103ce91773eacf7fc6bce001be02f043e2b**: **4,824 games, 543,888 legally replayed plies, 936 opening/colour/opponent contexts, and 68,905 shared-prefix positions**. Complete tables accompany the report as text files, each below 1 MB.

Within a recorded configuration, Regicide's searched choices differ at **six prefix decisions**: nine disagreeing game pairs out of 17,083 comparable searched-move pairs. Its zero-node choices never differ: **0 of 40,540 pairs**. Opponents differ in **1,883 of 58,649 comparable pairs**. These are conditional pair-exposure counts, not independent games or future error probabilities.

Three KingsRow branches have both move variation and different results within a configuration: Regicide White on **#16 at ply 41, #43 at ply 23, and #71 at ply 21**. The complete mining list has 39 positions; the other 36 require a fixed-configuration check because their outcome contrast is supported only by pooled configurations.

The match-order CSV recount is KingsRow a 0/312/0, KingsRow b 1/311/0, Cake a 0/312/0, and the available Cake b half 0/156/0. The decisive new record is **#16 White, mo-kingsrow-b-h1, game 28**. However, neither the tagged E2 PDNs nor the matching old/new match-order PDNs are committed. Process-age association and the first different move are therefore unavailable, not zero.

## Definitions that affect interpretation

All W/D/L values are from Regicide's viewpoint. A node is identified by opponent/version, opening, our colour, starting FEN and the **complete move prefix from the start**. A transposition through a different prefix is a separate node. Captures are parsed legally and canonicalized by their effect (from, to, captured set), so equivalent capture spellings are not false move differences. FENs and piece counts come from the independent tools/common referee, with no engine search.

For n games with a next move at one node, there are n(n-1)/2 comparable pairs. Disagreement is that count minus the sum of k(k-1)/2 for each next move's k games. Tables below show **disagreeing pairs / comparable pairs**. The imposed first three ballot plies remain in the full map but are excluded from engine-choice rates. Comparisons later in a game are conditional on still having the same full prefix.

A zero-node move is a Regicide comment with nodes 0; a searched move has nodes >0. This is the requested book proxy, kept literal because a table answer can also have zero nodes. Missing counts are not zero. No eligible repeated Regicide decision in this corpus lacks the needed node category.

The delivered summary has two scopes:
- **within_configuration:** r11c/n6c validation, r11c/n8q validation, r11d/n6c validation, and the r11d/n6c targeted test kept separate.
- **pooled:** all supplied records, including book/net changes; a pooled difference does not establish randomness in one fixed configuration.

The corpus is unbalanced: Cake and KingsRow have two validation repeats per colour/configuration, but GuiNN has only one. GuiNN's within-configuration replication comes from selected targeted tests. Rankings therefore include exposure and configuration counts and are not strength rankings.

## 1. PR #73 scheduling review

The committed runner_test pairs passes. Thirty additional scripted integration cases crossed worker counts 1/2/3, together mode off/on and --pairs limits 1/2/3/5/6, on a two-round deck containing a normal opening, fixed A=B and fixed A=W rows.

Every valid pair.leg appeared exactly once; skipped fixed-colour legs were absent. In together mode each ordinary pair stayed on one worker and its EngineGamesA/B values were adjacent. The scripted engines returned only 11-15 and 22-18 and loaded no real engine. The 3-worker case was the explicitly requested runner test; no search workers ran.

A direct test of the actual take_task/take_second_leg functions covered the same boundaries and stop_scheduling/interruption before taking the second leg: **32 cases, zero failures**.

An integration test signalled only its own runner after Round 1.1 was recorded, while the second game's newgame handshake was held before any second-game search. Only Round 1.1 remained, with an interrupted summary; exit status was 0. The direct test separately checked the exact between-leg stop boundary, so this external signal's scheduling race is not being used to prove that branch.

With the option absent, single-worker Round, players, result, opening, Worker, EngineGamesA/B and termination fields match the pre-PR runner. Wall times need not match. With multiple workers, worker assignment and completion order are races in both versions.

Reading the loop confirms the tested behaviour: an even first leg reserves both task IDs; a fixed-White opening skips the even task and grants only the odd one; a fixed-Black opening reserves its skipped odd leg but never plays it. Both total_games/--pairs bounds and stop flags are checked before granting the second leg. The fallback task request checks the stop flags again.

### Proposed asymmetric-restart test for the integrator

Add these helpers inside runner_test.cpp's anonymous namespace, reusing its existing scratch(), fake_braces() and includes. This tests independent reset logic, not only tag names.

~~~cpp
// Proposed additions to tools/match/runner_test.cpp.
// Existing helpers scratch() and fake_braces() are reused.
int fake_asymmetric_crash() {
    std::cout << "id name asymmetric crash fixture\nready\n" << std::flush;
    int newgames = 0;  // includes the launch warm-up
    bool moved = false;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "newgame") {
            ++newgames;
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("position ", 0) == 0) {
            moved = line.find(" moves ") != std::string::npos;
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("go ", 0) == 0) {
            if (newgames == 3) return 0;  // second real game; only this engine dies
            std::cout << "bestmove " << (moved ? "22-18" : "11-15")
                      << " result unknown time 0\n" << std::flush;
        }
    }
    return 0;
}

int asymmetric_restart(const char* self, const char* match_exe) {
    const fs::path dir = scratch("asymmetric-restart");
    std::ofstream(dir / "opening.txt") << "B:W21-32:B1-12\n";
    const std::string a = "exec:\"" + std::string(self) + "\" --fake-asymmetric-crash";
    const std::string b = "exec:\"" + std::string(self) + "\" --fake-braces";
    const std::string command =
        "\"" + std::string(match_exe) + "\" -a '" + a + "' -b '" + b +
        "' --openings \"" + (dir / "opening.txt").string() +
        "\" --rounds 2 --max-plies 2 -c 1 -o \"" +
        (dir / "out").string() + "\" > /dev/null 2>&1";
    const int status = std::system(command.c_str());

    struct Counts { int a = -1, b = -1; };
    std::map<std::string, Counts> counts;
    std::ifstream in(dir / "out" / "games.pdn");
    std::string line, round;
    auto value = [&] {
        const auto first = line.find('"'), last = line.rfind('"');
        return line.substr(first + 1, last - first - 1);
    };
    int records = 0, crashes = 0;
    while (std::getline(in, line)) {
        if (line.rfind("[Round ", 0) == 0) {
            round = value();
            ++records;
        } else if (line.rfind("[EngineGamesA ", 0) == 0) {
            counts[round].a = std::stoi(value());
        } else if (line.rfind("[EngineGamesB ", 0) == 0) {
            counts[round].b = std::stoi(value());
        } else if (line.rfind("[Termination ", 0) == 0 && line.find("crashed") != std::string::npos) {
            ++crashes;
        }
    }
    const std::map<std::string, Counts> expected{
        {"1.1", {0, 0}}, {"1.2", {1, 1}},
        {"2.1", {0, 2}}, {"2.2", {1, 3}}
    };
    bool ok = status == 0 && records == 4 && crashes == 2 && counts.size() == 4;
    for (const auto& [r, want] : expected) {
        const auto at = counts.find(r);
        if (at == counts.end() || at->second.a != want.a || at->second.b != want.b) {
            std::cerr << "FAIL: round " << r << " expected EngineGamesA/B "
                      << want.a << "/" << want.b << '\n';
            ok = false;
        }
    }
    if (status != 0 || records != 4 || crashes != 2)
        std::cerr << "FAIL: status " << status << ", records " << records
                  << ", crash games " << crashes << '\n';
    fs::remove_all(dir);
    return ok ? 0 : 1;
}


~~~

Add dispatch cases:

~~~cpp
if (argc == 2 && what == "--fake-asymmetric-crash")
    return fake_asymmetric_crash();
if (argc == 3 && what == "asymmetric-restart")
    return asymmetric_restart(argv[0], argv[2]);
~~~

Register:

~~~cmake
add_test(NAME match_asymmetric_restart
         COMMAND runner_test asymmetric-restart $<TARGET_FILE:match>)
~~~

Four two-ply games on one worker must give A/B ages 0/0, 1/1, 0/2, 1/3. A alone dies in its second real game in each process lifetime. The proposal passes current master (exit 0) and fails a shared-reset mutation (exit 1, wrong ages in rounds 2.1 and 2.2). It was compiled in a private source copy; production sources were not edited.

## 2a. Our variation within configurations

| Opponent | Zero-node different/comparable pairs | Searched different/comparable pairs | Varying searched decisions |
|---|---:|---:|---:|
| Cake 1.89f | 0 / 16,554 | 4 / 9,678 | 4 |
| KingsRow 1.20, book mode 3 | 0 / 23,632 | 0 / 7,162 | 0 |
| GuiNN 2.06 | 0 / 354 | 5 / 243 | 2 |

The GuiNN row is a small selected sample. Zero observed searched differences against KingsRow does not establish that timed searching is deterministic generally. A 2–2 split in four games contributes four disagreeing pairs, so nine pair disagreements correspond to six decisions.

### Zero-node moves by ply band

| Opponent | <=20 | 21-40 | 41-60 | >60 |
|---|---|---|---|---|
| cake | 0 / 8,164 | 0 / 7,090 | 0 / 1,216 | 0 / 84 |
| kingsrow | 0 / 10,862 | 0 / 10,131 | 0 / 2,376 | 0 / 263 |
| guinn | 0 / 243 | 0 / 99 | 0 / 12 | — (no repeated exposure) |

### Searched moves by ply band

| Opponent | <=20 | 21-40 | 41-60 | >60 |
|---|---|---|---|---|
| cake | 0 / 24 | 0 / 556 | 2 / 4,492 | 2 / 4,606 |
| kingsrow | 0 / 17 | 0 / 636 | 0 / 3,855 | 0 / 2,654 |
| guinn | 0 / 41 | 4 / 52 | 1 / 85 | 0 / 65 |

### Zero-node moves by piece count before the move

| Opponent | 17-24 | 9-16 | 5-8 | 1-4 |
|---|---|---|---|---|
| cake | 0 / 9,392 | 0 / 7,162 | — (no repeated exposure) | — (no repeated exposure) |
| kingsrow | 0 / 13,041 | 0 / 10,591 | — (no repeated exposure) | — (no repeated exposure) |
| guinn | 0 / 286 | 0 / 68 | — (no repeated exposure) | — (no repeated exposure) |

### Searched moves by piece count before the move

| Opponent | 17-24 | 9-16 | 5-8 | 1-4 |
|---|---|---|---|---|
| cake | 0 / 37 | 4 / 740 | 0 / 7,942 | 0 / 959 |
| kingsrow | 0 / 24 | 0 / 1,109 | 0 / 5,571 | 0 / 458 |
| guinn | 0 / 50 | 5 / 48 | 0 / 130 | 0 / 15 |

All six varying searched decisions have 9–16 pieces; all their games drew:

| Configuration | Opponent | Opening / our colour | Ply | Pieces | Choices |
|---|---|---|---:|---:|---|
| r11d/n6c targeted | Cake | #141 B | 99 | 10 | 31-26 once; 32-27 once |
| r11c/n8q validation | Cake | #78 W | 46 | 10 | 29-25 once; 3-8 once |
| r11d/n6c validation | Cake | #93 W | 72 | 10 | 24-19 once; 5-9 once |
| r11c/n8q validation | Cake | #96 B | 47 | 10 | 12-16 once; 18-22 once |
| r11d/n6c targeted | GuiNN | #60 B | 51 | 10 | 12-16 once; 2-7 once |
| r11d/n6c targeted | GuiNN | #71 B | 23 | 16 | 1-5 twice; 7-11 twice |

The branch tables provide FENs and source rounds. These observations measure variation; they do not identify whether the clock or retained within-game search state caused it.

Across all pooled configurations, zero-node disagreements are Cake 16, KingsRow 16 and GuiNN 2, versus zero within each configuration. Pooled searched disagreements are 209, 103 and 27. Book/net changes must not be labelled our engine's own randomness. Both scopes and all denominators are in [variation-summary.tsv](tables/variation-summary.tsv).

## 2b. Opponent variation

### By ply band, within a configuration

| Opponent | <=20 | 21-40 | 41-60 | >60 |
|---|---|---|---|---|
| cake | 88 / 8,249 | 234 / 7,762 | 176 / 5,797 | 90 / 4,652 |
| kingsrow | 34 / 11,151 | 385 / 10,925 | 475 / 6,478 | 366 / 3,008 |
| guinn | 26 / 323 | 6 / 147 | 3 / 99 | 0 / 58 |

Totals: Cake 588/26,460 (573 varying prefix/configuration decisions), KingsRow 1,260/31,562 (961 decisions), GuiNN 35/627 (13 decisions). Pooled totals are Cake 2,929/125,950, KingsRow 5,261/126,852, GuiNN 931/11,327. The summary includes both scopes and an additional piece-count cut.

### Top 30 opening/colour contexts per opponent

Distinct opponent lines are distinct sequences of that opponent's moves through ply 40, truncated when a game ends sooner. Full lines also include our moves. Different opponent strings can be reactions to earlier Regicide choices; the shared-prefix analysis above is what holds the complete preceding sequence fixed. More targeted repetitions create more chances to observe lines.

The complete ranking is in [opponent-lines40-all.tsv](tables/opponent-lines40-all.tsv).

#### cake

| Opening | Our colour | Games | Reached 40 | Configurations | Opponent lines | Full lines |
|---|---|---|---|---|---|---|
| ACF #112 | W | 6 | 6 | 3 | 6 | 6 |
| ACF #118 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #133 Octopus | B | 6 | 6 | 3 | 6 | 6 |
| ACF #139 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #152 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #166 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #24 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #37 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #43 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #61 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #70 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #75 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #93 | B | 10 | 10 | 4 | 5 | 5 |
| ACF #120 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #121 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #124 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #131 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #136 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #142 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #146 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #146 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #147 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #157 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #162 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #19 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #25 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #29 | W | 6 | 6 | 3 | 5 | 6 |
| ACF #37 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #50 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #64 | B | 6 | 6 | 3 | 5 | 5 |

#### kingsrow

| Opening | Our colour | Games | Reached 40 | Configurations | Opponent lines | Full lines |
|---|---|---|---|---|---|---|
| ACF #71 Diabolical Denny | B | 16 | 16 | 4 | 11 | 11 |
| ACF #34 Twilight Zone | B | 16 | 16 | 4 | 7 | 7 |
| ACF #100 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #11 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #150 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #169 | W | 6 | 6 | 3 | 6 | 6 |
| ACF #24 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #49 | W | 6 | 6 | 3 | 6 | 6 |
| ACF #70 | B | 6 | 6 | 3 | 6 | 6 |
| ACF #108 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #110 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #12 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #120 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #121 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #129 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #139 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #143 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #150 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #173 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #18 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #19 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #22 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #30 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #49 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #50 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #66 | W | 6 | 6 | 3 | 5 | 5 |
| ACF #69 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #86 Octopus | W | 6 | 6 | 3 | 5 | 5 |
| ACF #99 | B | 6 | 6 | 3 | 5 | 5 |
| ACF #99 | W | 6 | 6 | 3 | 5 | 5 |

#### guinn

| Opening | Our colour | Games | Reached 40 | Configurations | Opponent lines | Full lines |
|---|---|---|---|---|---|---|
| ACF #9 Wilderness I | B | 7 | 7 | 4 | 7 | 7 |
| ACF #34 Twilight Zone | B | 7 | 7 | 4 | 6 | 6 |
| ACF #62 Gemini I | B | 7 | 7 | 4 | 6 | 6 |
| ACF #60 Black Hole | B | 7 | 7 | 4 | 5 | 5 |
| ACF #71 Diabolical Denny | B | 7 | 7 | 4 | 5 | 5 |
| ACF #93 | B | 7 | 7 | 4 | 5 | 5 |
| ACF #141 | B | 7 | 7 | 4 | 4 | 4 |
| ACF #53 | W | 7 | 7 | 4 | 3 | 3 |
| ACF #10 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #10 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #101 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #102 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #103 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #103 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #104 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #105 Gemini II | B | 3 | 3 | 3 | 3 | 3 |
| ACF #105 Gemini II | W | 3 | 3 | 3 | 3 | 3 |
| ACF #106 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #107 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #107 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #112 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #113 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #115 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #116 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #116 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #117 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #117 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #118 | B | 3 | 3 | 3 | 3 | 3 |
| ACF #118 | W | 3 | 3 | 3 | 3 | 3 |
| ACF #119 | B | 3 | 3 | 3 | 3 | 3 |

### First varying opponent choice and process age

**Unavailable in the committed PDNs.** None of the 4,824 selected PDNs has Worker, EngineGames or EngineGamesA/B. Newer E2/match-order directories supply summaries and CSVs, not their PDNs. Round numbers and completion order cannot recover worker/process identity, so no 0/1/2+ bucket was inferred.

[process-age-first-variation.tsv](tables/process-age-first-variation.tsv) contains the schema and no observations. Its grain is one tagged game at the first varying opponent node on its own prefix path, with node, move, result, age, worker and round. These launch scripts configure Regicide as engine A, so EngineGamesB would give opponent age; older EngineGames would be used only with its common-counter limitation. No observed age association or causal process-state claim follows from an empty table.

## 2c. Mining starting positions

Sorted by games at the node. A position requires at least two opponent moves and different result proportions by move. “Within config” means move variation **and** a result difference occur in at least one configuration. Pooled-only entries are leads for a frozen-configuration check, not evidence to accept a forced move.

| Opponent | Opening | Our colour | Ply | Games | FEN | Moves: count (Regicide W/D/L) | Within config |
|---|---|---|---|---|---|---|---|
| kingsrow | ACF #16 Wilderness II | W | 41 | 6 | B:W6,15,20,30,31:B4,5,9,12,K32 | 32-28:4(W0/D4/L0); 9-14:2(W2/D0/L0) | yes |
| kingsrow | ACF #43 | W | 23 | 6 | B:W21,25,26,27,28,30,31:B1,4,6,7,8,14,19 | 8-11:3(W0/D3/L0); 8-12:3(W3/D0/L0) | yes |
| kingsrow | ACF #71 Diabolical Denny | W | 21 | 6 | B:W22,23,25,27,28,31,32:B1,4,5,8,10,12,13 | 5-9:1(W0/D1/L0); 8-11:5(W1/D4/L0) | yes |
| kingsrow | ACF #71 Diabolical Denny | W | 25 | 5 | B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13 | 10-14:1(W1/D0/L0); 5-9:4(W0/D4/L0) | pooled only |
| guinn | ACF #10 | W | 7 | 3 | B:W18,21,22,23,24,25,26,27,28,30,31,32:B1,2,3,4,5,6,7,10,11,12,13,16 | 10-15:2(W1/D1/L0); 16-20:1(W0/D1/L0) | pooled only |
| guinn | ACF #101 | W | 7 | 3 | B:W21,22,23,24,25,27,28,29,30,31,32:B1,2,3,4,5,6,7,9,12,15,16 | 6-10:2(W0/D2/L0); 9-14:1(W1/D0/L0) | pooled only |
| guinn | ACF #111 | B | 4 | 3 | W:W17,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,9,10,11,12,15 | 17-13:2(W0/D2/L0); 25-21:1(W1/D0/L0) | pooled only |
| guinn | ACF #116 | B | 4 | 3 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,9,10,11,12,15 | 17-14:1(W0/D1/L0); 25-22:2(W2/D0/L0) | pooled only |
| guinn | ACF #117 | W | 7 | 3 | B:W14,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,12,13,15 | 15-18:1(W1/D0/L0); 8-11:2(W0/D2/L0) | pooled only |
| guinn | ACF #12 | B | 4 | 3 | W:W18,21,22,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,10,11,12,13 | 24-20:1(W1/D0/L0); 26-23:1(W0/D1/L0); 27-23:1(W0/D1/L0) | pooled only |
| guinn | ACF #124 | W | 7 | 3 | B:W20,21,22,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,9,10,15,16 | 7-11:2(W0/D2/L0); 9-14:1(W1/D0/L0) | pooled only |
| guinn | ACF #129 | B | 24 | 3 | W:W14,18,19,21,23,25,27,31,32:B2,3,5,6,7,9,11,12,20 | 27-24:1(W1/D0/L0); 31-26:2(W0/D2/L0) | pooled only |
| guinn | ACF #134 | W | 15 | 3 | B:W13,22,23,24,27,28,29,30,31,32:B1,2,3,6,7,8,11,12,14,20 | 14-17:1(W1/D0/L0); 7-10:2(W1/D1/L0) | pooled only |
| guinn | ACF #140 | W | 11 | 3 | B:W17,18,21,22,25,26,27,28,29,31,32:B1,2,4,5,6,7,8,9,11,12,19 | 11-16:2(W2/D0/L0); 12-16:1(W0/D1/L0) | pooled only |
| guinn | ACF #141 | W | 13 | 3 | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 11-15:1(W0/D1/L0); 16-20:1(W1/D0/L0); 4-8:1(W1/D0/L0) | pooled only |
| guinn | ACF #153 Black Widow | B | 18 | 3 | W:W17,18,20,22,23,28,29,30,31,32:B1,2,3,5,7,11,12,13,15,16 | 17-14:1(W1/D0/L0); 29-25:1(W0/D1/L0); 32-27:1(W1/D0/L0) | pooled only |
| guinn | ACF #16 Wilderness II | W | 9 | 3 | B:W14,18,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,12,13,16 | 1-6:2(W2/D0/L0); 7-11:1(W0/D1/L0) | pooled only |
| guinn | ACF #162 | W | 13 | 3 | B:W9,17,21,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,10,20 | 5x14:1(W0/D1/L0); 6x13x22:2(W1/D1/L0) | pooled only |
| guinn | ACF #174 Skunk | W | 11 | 3 | B:W18,20,21,23,24,25,26,28,30,31,32:B1,2,3,4,5,7,9,10,11,12,16 | 10-15:2(W0/D2/L0); 9-14:1(W1/D0/L0) | pooled only |
| guinn | ACF #23 | W | 5 | 3 | B:W19,21,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,6,7,8,9,10,11,12,13 | 10-14:2(W2/D0/L0); 11-15:1(W0/D1/L0) | pooled only |
| guinn | ACF #25 | B | 4 | 3 | W:W19,21,22,23,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,11,12,13,14 | 22-18:1(W1/D0/L0); 27-24:2(W0/D2/L0) | pooled only |
| guinn | ACF #25 | W | 15 | 3 | B:W17,19,21,23,24,27,29,30,31,32:B1,2,3,4,7,9,10,12,14,15 | 4-8:2(W2/D0/L0); 9-13:1(W0/D1/L0) | pooled only |
| guinn | ACF #34 Twilight Zone | W | 13 | 3 | B:W21,22,24,25,26,27,28,29,30,31:B1,2,4,6,7,9,10,12,13,15 | 7-11:1(W0/D1/L0); 9-14:2(W2/D0/L0) | pooled only |
| guinn | ACF #50 | W | 11 | 3 | B:W17,19,24,25,27,28,29,30,31,32:B1,2,3,4,5,6,8,10,12,18 | 2-7:2(W2/D0/L0); 8-11:1(W0/D1/L0) | pooled only |
| guinn | ACF #6 Inferno | W | 9 | 3 | B:W18,21,23,24,25,27,28,29,30,31,32:B1,2,3,4,7,8,9,11,12,13,14 | 11-16:1(W1/D0/L0); 7-10:2(W0/D2/L0) | pooled only |
| guinn | ACF #60 Black Hole | W | 19 | 3 | B:W14,21,22,27,29,30,31,32:B2,3,4,5,7,11,12,13 | 11-15:1(W1/D0/L0); 4-8:2(W0/D2/L0) | pooled only |
| guinn | ACF #62 Gemini I | W | 11 | 3 | B:W18,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,8,9,10,12,16 | 1-6:2(W2/D0/L0); 3-7:1(W0/D1/L0) | pooled only |
| guinn | ACF #64 | W | 5 | 3 | B:W18,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,10,11,12,14 | 1-6:2(W2/D0/L0); 11-15:1(W0/D1/L0) | pooled only |
| guinn | ACF #66 | W | 9 | 3 | B:W19,21,23,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,9,12,14 | 4-8:2(W2/D0/L0); 7-11:1(W0/D1/L0) | pooled only |
| guinn | ACF #71 Diabolical Denny | W | 9 | 3 | B:W15,21,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,12,14 | 12-16:2(W2/D0/L0); 3-7:1(W0/D1/L0) | pooled only |
| guinn | ACF #78 | W | 5 | 3 | B:W19,21,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,8,9,12,14,16 | 6-10:2(W0/D2/L0); 7-10:1(W1/D0/L0) | pooled only |
| guinn | ACF #87 Tyne | W | 11 | 3 | B:W14,19,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,11,12,13,18 | 4-8:1(W1/D0/L0); 6-10:2(W0/D2/L0) | pooled only |
| guinn | ACF #9 Wilderness I | W | 7 | 3 | B:W14,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,12,13,16 | 16-20:1(W0/D1/L0); 6-9:2(W1/D1/L0) | pooled only |
| guinn | ACF #92 | B | 4 | 3 | W:W17,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,15 | 17-14:1(W1/D0/L0); 24-20:1(W1/D0/L0); 25-22:1(W0/D1/L0) | pooled only |
| guinn | ACF #96 Skull-Cracker | W | 15 | 3 | B:W14,21,25,26,27,28,29,31,32:B1,2,3,4,7,8,9,12,18 | 1-5:2(W2/D0/L0); 9-13:1(W0/D1/L0) | pooled only |
| guinn | ACF #10 | W | 17 | 2 | B:W14,18,19,21,22,23,26,27,30,31,32:B1,2,3,4,5,7,9,12,13,16,20 | 1-6:1(W0/D1/L0); 4-8:1(W1/D0/L0) | pooled only |
| guinn | ACF #134 | W | 17 | 2 | B:W13,18,23,24,27,28,29,30,31,32:B1,2,3,6,8,10,11,12,14,20 | 1-5:1(W0/D1/L0); 14-17:1(W1/D0/L0) | pooled only |
| guinn | ACF #162 | W | 19 | 2 | B:W17,21,22,27,28,30,31,32:B1,2,3,4,9,10,11,20 | 11-15:1(W0/D1/L0); 4-8:1(W1/D0/L0) | pooled only |
| guinn | ACF #9 Wilderness I | W | 13 | 2 | B:W14,21,24,25,26,27,28,30,31,32:B1,2,3,4,5,7,11,12,13,16 | 11-15:1(W0/D1/L0); 4-8:1(W1/D0/L0) | pooled only |

The three strongest cases are:
- #16 W, ply 41: 32-28 drew four times; 9-14 lost twice. The contrast occurs within r11c/n8q and within r11d/n6c.
- #43 W, ply 23: 8-11 drew three times; 8-12 lost three times. Each of the three validation configurations contains the contrast.
- #71 W, ply 21: 5-9 drew once; 8-11 lost once and drew four times. Within r11d/n6c it is one draw via 5-9 versus one loss via 8-11; the other draws come from earlier configurations.

[mining-evidence.tsv](tables/mining-evidence.tsv) gives every source game, move, outcome and configuration. Candidate details also retain run/round references. Check these positions against the current book and protected-decision rules before nomination. Sample outcome associations are not proofs of move values.

## 3. Match-order result recount

These counts come from individual committed CSV rows, with unique game and pair/colour identities checked.

| Run | Games | Regicide W | D | L |
|---|---|---|---|---|
| mo-cake-a-h1 | 156 | 0 | 156 | 0 |
| mo-cake-a-h2 | 156 | 0 | 156 | 0 |
| mo-cake-b-h1 | 156 | 0 | 156 | 0 |
| mo-kingsrow-a-h1 | 156 | 0 | 156 | 0 |
| mo-kingsrow-a-h2 | 156 | 0 | 156 | 0 |
| mo-kingsrow-b-h1 | 156 | 1 | 155 | 0 |
| mo-kingsrow-b-h2 | 156 | 0 | 156 | 0 |

There are 1,092 committed rows: one win, 1,091 draws and no losses. KingsRow has both full repeats; Cake a is complete, but Cake b has only h1. There is no committed completed GuiNN match-order result file here. Seven half-runs must not be labelled a completed three-opponent validation.

The win is **mo-kingsrow-b-h1, game 28, pair 14, #16 Wilderness II, Regicide White**, 217 engine plies after the ballot, no legal moves. The opening was mapped through the 156 unique half-file/row-to-ballot mappings in older PDN Opening tags; there were no conflicting mappings.

The earlier same-book/same-net comparison is r11d/n8q. Committed RESULTS.md and wp11d/v11d_n8q_report.md report 0/312/0 for each KingsRow and Cake repeat. Against that **reported** all-draw baseline, #16 White is the one result change in the available new rows. This is not an independently recounted old/new game join: the old r11d/n8q raw PDNs and CSVs are absent from these committed locations.

The available earlier r11d PDNs use n6c. They were not substituted for n8q. [match-order-game-comparison.tsv](tables/match-order-game-comparison.tsv) lists every new row and labels the old result as reported. First differing ply and mover remain explicitly unavailable because the matching old/new PDNs are absent. They were not guessed from game length or a summary.

Even with PDNs this would be observational: old r11d/n8q uses the 5c433dc DLL; new matches use 1c6e997, including the game-history root fixes. The book and net match, but scheduling is not the sole code-version difference.

## Evidence and reproducibility

[sources.tsv](tables/sources.tsv) identifies all source paths, configurations, content SHA-256 values, counts and tag coverage. Included cohorts: 1,560 r11c/n6c validation games, 1,560 r11c/n8q, 1,560 r11d/n6c and 144 targeted r11d/n6c games. [games.tsv](tables/games.tsv) is the individual-game index. The requested opponent versions are Cake 1.89f, KingsRow 1.20 with mode 3 established by the run records, and GuiNN 2.06; KingsRow's bracketed process-name suffix is not a different opponent.

Exact game identity is run plus Round. Identical played lines in different games were kept as legitimate repeats. There were no duplicate/conflicting identities, unfinished results, off-board terminations or replay failures. Every move was replayed, including captures and FEN starts.

An independent pass over the saved shared-prefix rows recomputed pooled pair counts/disagreements and reconciled them with the summary. Non-root shared prefixes retain a parent present in the map, so the complete history can be reconstructed without repeating it in each row.

The following are the full map and branch tables, split below 900 KB per UTF-8 file:

### All shared prefixes

- [shared-prefixes-001.tsv](tables/shared-prefixes-001.tsv)
- [shared-prefixes-002.tsv](tables/shared-prefixes-002.tsv)
- [shared-prefixes-003.tsv](tables/shared-prefixes-003.tsv)
- [shared-prefixes-004.tsv](tables/shared-prefixes-004.tsv)
- [shared-prefixes-005.tsv](tables/shared-prefixes-005.tsv)
- [shared-prefixes-006.tsv](tables/shared-prefixes-006.tsv)
- [shared-prefixes-007.tsv](tables/shared-prefixes-007.tsv)
- [shared-prefixes-008.tsv](tables/shared-prefixes-008.tsv)
- [shared-prefixes-009.tsv](tables/shared-prefixes-009.tsv)
- [shared-prefixes-010.tsv](tables/shared-prefixes-010.tsv)

### Varying decisions within configurations

- [within-configuration-branches-001.tsv](tables/within-configuration-branches-001.tsv)

### Mining candidate details

- [mining-candidates-001.tsv](tables/mining-candidates-001.tsv)

No search or real-engine match was used for the map. Runner tests used only scripted fixtures, at low priority inside ~/regicide-review-a. Builds used at most two jobs. No other agent's directory, process or lock file was touched. The ledger was read first and re-read; round-2 findings handed to owners were not reported again.

Only this report and its text tables, each under 1 MB, are intended for the shared reports branch. Source extracts, scripts, binaries, fixtures and databases remain local.

Source paths in the tables are relative to [experiments/booklab at the reviewed commit](https://github.com/AdminGodZ/Regicide/tree/33e89103ce91773eacf7fc6bce001be02f043e2b/experiments/booklab).
