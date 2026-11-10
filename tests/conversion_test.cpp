// Conversion of database wins without distance information (Engine::go, conversion search). Builds a small
// database (WLD and MTC up to 4 pieces), caps the pieces for which the engine may use MTC, and plays won positions
// out under the referee's rules (threefold repetition and 79 plies without capture or man move are draws) against
// the database policy: an engine with the full MTC, which plays every decided position by Engine::egdb_root_move.
// That policy always realises a win but is not MTC-optimal (see egdb_root_move), so the reference game it plays
// against itself is a baseline, not the shortest win or the longest defence.
//
//   conversion_test                          the ctest check: audit positions, the winner keeps the win (also when
//                                            every winning move repeats a position), a sample of 3- and 4-piece wins
//   conversion_test [options]                measurements:
//     --dir D        load the database from D instead of building 2..4 pieces into a temporary directory
//     --build N      with --dir: build 2..N pieces (WLD and MTC) into D first if it has no N-piece tables
//     --pieces a,b   piece counts to sample (default 3,4)
//     --sample N     positions per piece count (default 100; a quarter for each colour and side to move)
//     --nodes N      engine nodes per move (default 20000)
//     --cap N        the engine uses MTC only with at most N pieces (default 0)
//     --role win|lose|both  the engine converts the win, defends the loss, or both (default win)
//     --threads N    engine search threads (default 1)
//     --oracle-nodes N  where the database has no MTC, the opponent searches N nodes per move (default: depth 1)
//     --seed N       sampling seed (default 1)
//     --kind K       sample only kings|one|both: kings only, men on one side, men on both sides (default any)
//     --list         print every game
//     --fen F        play only this position (the engine wins, or defends with --role lose), move by move
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "egdb/build.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;
using egdb::Wld;

namespace {

int failures = 0;
bool trace = false;

void check(bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    std::fflush(stdout);
    failures += !ok;
}

Wld probe(const Position& p) { return egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move()); }

bool is_conversion(const Position& p, const Move& m) { return m.is_capture() || !(p.kings() & bit(m.from)); }

struct Game {
    enum Result { Won, Repetition, NoProgress, GaveAway, TooLong } result = TooLong;
    int plies = 0;
};

const char* result_name(Game::Result r) {
    static const char* names[] = {"won", "repetition", "no progress", "gave away the win", "too long"};
    return names[r];
}

// Plays `start` out; `players[c]` moves for colour c. `winner` is the side the database says wins. Stops when the
// loser has no move (Won), on a referee draw, or when a move leaves the winner without a database win.
Game play(const Position& start, Color winner, Engine* players[2], const Limits limits[2]) {
    Game g;
    Position pos = start;
    std::vector<Position> history, window{pos};
    for (auto c : {BLACK, WHITE}) players[c]->new_game();
    for (g.plies = 0; g.plies < 400; ++g.plies) {
        MoveList legal;
        generate_moves(pos, legal);
        if (legal.empty()) {
            g.result = pos.side_to_move() == winner ? Game::GaveAway : Game::Won;
            return g;
        }
        Engine& e = *players[pos.side_to_move()];
        e.set_position(pos, history);
        const SearchResult res = e.go(limits[pos.side_to_move()]);
        const Move m = res.best;
        if (trace)
            std::printf("  %3d %-24s %2d-%-2d score %6d depth %2d nodes %llu\n", g.plies, pos.fen().c_str(),
                        to_pdn(m.from), to_pdn(m.to), res.score, res.depth, (unsigned long long)res.nodes);
        if (std::find(legal.begin(), legal.end(), m) == legal.end()) std::abort();
        const bool conversion = is_conversion(pos, m);
        history.push_back(pos);
        pos.make_move(m);
        if (probe(pos) != (pos.side_to_move() == winner ? Wld::Win : Wld::Loss)) {
            g.result = Game::GaveAway;
            ++g.plies;
            return g;
        }
        if (conversion) {
            window.assign(1, pos);
        } else {
            window.push_back(pos);
            if (std::count(window.begin(), window.end(), pos) >= 3) {
                g.result = Game::Repetition;
                ++g.plies;
                return g;
            }
            if (pos.reversible_plies() >= NO_PROGRESS_PLIES) {
                g.result = Game::NoProgress;
                ++g.plies;
                return g;
            }
        }
    }
    return g;
}

// Kinds of positions, reported separately: progress in real games is made mostly with men.
enum Kind { KINGS_ONLY, MEN_ONE_SIDE, MEN_BOTH_SIDES, ANY_KIND };
const char* kind_name(int k) {
    static const char* names[] = {"kings only", "men on one side", "men on both sides"};
    return names[k];
}
Kind kind_of(const Position& p) {
    return Kind(bool(p.men(BLACK)) + bool(p.men(WHITE)));
}

// Random quiet decided positions with `n` pieces: the side to move has no capture and the position is won or lost.
// `kind` restricts them to one kind of position.
std::vector<Position> sample(int n, int count, std::mt19937_64& rng, Kind kind = ANY_KIND) {
    std::vector<Position> out;
    int bucket[4] = {};  // [winner is black][winner to move]
    while (int(out.size()) < count) {
        Bitboard side[2] = {}, kings = 0, occ = 0;
        for (int i = 0; i < n; ++i) {
            const Color c = i == 0 ? BLACK : i == 1 ? WHITE : Color(rng() & 1);
            const bool king = rng() & 1;
            int sq;
            do sq = int(rng() % 32);
            while ((occ & bit(sq)) || (!king && (bit(sq) & promo_row(c))));
            occ |= bit(sq);
            side[c] |= bit(sq);
            if (king) kings |= bit(sq);
        }
        Position p;
        p.set(side[BLACK], side[WHITE], kings, Color(rng() & 1));
        if (has_capture(p, p.side_to_move()) || (kind != ANY_KIND && kind_of(p) != kind)) continue;
        if (!quiet_targets(p, p.side_to_move())) continue;  // the game is already over
        const Wld r = probe(p);
        if (r != Wld::Win && r != Wld::Loss) continue;
        const Color winner = r == Wld::Win ? p.side_to_move() : ~p.side_to_move();
        int& b = bucket[2 * (winner == BLACK) + (r == Wld::Win)];
        if (b >= (count + 3) / 4) continue;
        ++b;
        out.push_back(p);
    }
    return out;
}

struct Options {
    std::string dir;
    int build = 0;
    std::vector<int> pieces{3, 4};
    int sample = 100;
    std::uint64_t nodes = 20000;
    int cap = 0;
    std::string role = "win";
    int threads = 1;
    std::uint64_t oracle_nodes = 0;
    std::uint64_t seed = 1;
    bool list = false;
    Kind kind = ANY_KIND;
    std::string fen;
};

struct Engines {
    Engine engine, oracle;
    Limits engine_limits, oracle_limits;

    Engines(const Options& o) {
        for (Engine* e : {&engine, &oracle}) {
            e->set_hash_mb(16);
            e->set_egdb_pieces(egdb::max_pieces());
        }
        engine.set_threads(o.threads);
        engine.set_egdb_mtc_pieces(o.cap);
        engine_limits.nodes = o.nodes;
        engine_limits.use_book = false;
        oracle_limits.depth = o.oracle_nodes ? MAX_PLY - 1 : 1;
        oracle_limits.nodes = o.oracle_nodes;
        oracle_limits.use_book = false;
    }
    // Game with the engine playing the winner (`engine_wins`) or the loser, and the database policy (`oracle`) the
    // other side.
    Game play(const Position& p, bool engine_wins) {
        const Color winner = probe(p) == Wld::Win ? p.side_to_move() : ~p.side_to_move();
        Engine* players[2];
        Limits limits[2];
        players[winner] = engine_wins ? &engine : &oracle;
        players[~winner] = engine_wins ? &oracle : &engine;
        limits[winner] = engine_wins ? engine_limits : oracle_limits;
        limits[~winner] = engine_wins ? oracle_limits : engine_limits;
        return ::play(p, winner, players, limits);
    }
    // Reference game: the database policy on both sides.
    Game reference(const Position& p) {
        const Color winner = probe(p) == Wld::Win ? p.side_to_move() : ~p.side_to_move();
        Engine* players[2] = {&oracle, &oracle};
        const Limits limits[2] = {oracle_limits, oracle_limits};
        return ::play(p, winner, players, limits);
    }
};

int percentile(std::vector<int> v, int pct) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, v.size() * pct / 100)];
}

// The engine converts (role win) or defends (role lose) every sampled position; prints a summary per piece count.
void measure(Engines& en, const Options& o, bool engine_wins) {
    std::mt19937_64 rng(o.seed);
    for (int n : o.pieces) {
        const auto start = std::chrono::steady_clock::now();
        const std::vector<Position> positions = sample(n, o.sample, rng, o.kind);
        std::map<Game::Result, int> results;
        int kind_games[3] = {}, kind_won[3] = {};
        std::vector<int> excess, reference_len, ratio_pct;
        int at_most_reference = 0, reference_lost = 0;
        double sum_engine = 0, sum_reference = 0;
        for (const Position& p : positions) {
            // The database policy always wins; the reference can only fail where the database has no MTC.
            const Game opt = en.reference(p);
            if (opt.result != Game::Won && egdb::max_mtc_pieces() >= n) std::abort();
            reference_lost += opt.result != Game::Won;
            const Game g = en.play(p, engine_wins);
            ++results[g.result];
            ++kind_games[kind_of(p)];
            kind_won[kind_of(p)] += g.result == Game::Won;
            sum_engine += g.plies;
            sum_reference += opt.plies;
            reference_len.push_back(opt.plies);
            if (g.result == Game::Won || !engine_wins) {
                excess.push_back(g.plies - opt.plies);
                ratio_pct.push_back(opt.plies ? 100 * g.plies / opt.plies : 100);
                at_most_reference += g.plies <= opt.plies;
            }
            if (o.list || (engine_wins && g.result != Game::Won))
                std::printf("  %-28s reference %3d  engine %3d  %s\n", p.fen().c_str(), opt.plies, g.plies,
                            result_name(g.result));
        }
        const double secs =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::printf("%s %d pieces, %zu positions, %llu nodes/move, MTC cap %d: ", engine_wins ? "convert" : "defend",
                    n, positions.size(), (unsigned long long)o.nodes, o.cap);
        for (const auto& [r, c] : results) std::printf("%s %d, ", result_name(r), c);
        if (reference_lost) std::printf("reference game not won %d, ", reference_lost);
        std::printf("mean plies: engine %.1f, reference %.1f", sum_engine / positions.size(),
                    sum_reference / positions.size());
        std::printf("\n  won by kind:");
        for (int k = 0; k < 3; ++k) std::printf(" %s %d/%d", kind_name(k), kind_won[k], kind_games[k]);
        std::printf("\n  reference length median %d p90 %d max %d; engine minus reference: median %d p90 %d max %d; "
                    "engine/reference %%: median %d p90 %d; at most reference %d; %.1f s\n",
                    percentile(reference_len, 50), percentile(reference_len, 90), percentile(reference_len, 100),
                    percentile(excess, 50), percentile(excess, 90), percentile(excess, 100),
                    percentile(ratio_pct, 50), percentile(ratio_pct, 90), at_most_reference, secs);
        if (engine_wins)
            check(results[Game::Won] == int(positions.size()),
                  "converted " + std::to_string(results[Game::Won]) + " of " + std::to_string(positions.size()) +
                      " " + std::to_string(n) + "-piece wins");
        else
            check(results[Game::Won] == int(positions.size()), "the defender loses every game, as it must");
    }
}

// The winner never moves into a drawn or lost position, even at the smallest depths: in every sampled position
// where some move gives the win away, a depth 1 and a depth 2 search pick a move that keeps it. Engine::go keeps
// only winning root moves anyway; listing every move as an analysis move skips that, so the scores decide.
void test_stays_won(const Options& o) {
    Engine engine;
    engine.set_egdb_pieces(egdb::max_pieces());
    engine.set_egdb_mtc_pieces(o.cap);
    std::mt19937_64 rng(o.seed + 99);
    int tempting = 0, kept = 0;
    for (int n : o.pieces)
        for (const Position& p : sample(n, 400, rng)) {
            if (probe(p) != Wld::Win) continue;
            MoveList legal;
            generate_moves(p, legal);
            bool can_throw = false;
            for (const Move& m : legal) {
                Position c = p;
                c.make_move(m);
                MoveList replies;
                generate_moves(c, replies);
                can_throw |= !replies.empty() && probe(c) != Wld::Loss;
            }
            if (!can_throw) continue;
            ++tempting;
            bool ok = true;
            for (int depth : {1, 2}) {
                Limits l;
                l.depth = depth;
                l.use_book = false;
                l.searchmoves.assign(legal.begin(), legal.end());
                engine.new_game();
                engine.set_position(p);
                Position c = p;
                c.make_move(engine.go(l).best);
                MoveList replies;
                generate_moves(c, replies);
                ok &= replies.empty() || probe(c) == Wld::Loss;
            }
            kept += ok;
        }
    check(tempting > 20 && kept == tempting, "the winner keeps the win at depth 1 and 2 in " + std::to_string(kept) +
                                                 " of " + std::to_string(tempting) +
                                                 " positions where a move gives it away");
}

// The game has visited the position after every winning move once already (a second visit is not yet the
// threefold repetition), so in the search they all repeat a position; they must still rank above every move that
// gives the win away. And a winning move whose position occurred twice already (the third occurrence draws) must
// give way to one that occurred once. Searched as analysis moves, so the scores decide (see test_stays_won).
void test_repeated_wins(const Options& o) {
    Engine engine;
    engine.set_egdb_pieces(egdb::max_pieces());
    engine.set_egdb_mtc_pieces(o.cap);
    std::mt19937_64 rng(o.seed + 7);
    int cases = 0, kept = 0, threefold_cases = 0, threefold_avoided = 0;
    for (const Position& p : sample(3, 400, rng, KINGS_ONLY)) {
        if (probe(p) != Wld::Win) continue;
        MoveList legal;
        generate_moves(p, legal);
        std::vector<Position> won_children;
        std::vector<Move> winning;
        bool can_throw = false;
        for (const Move& m : legal) {
            Position c = p;
            c.make_move(m);
            MoveList replies;
            generate_moves(c, replies);
            if (!replies.empty() && probe(c) == Wld::Loss) {
                won_children.push_back(c);
                winning.push_back(m);
            }
            can_throw |= !replies.empty() && probe(c) != Wld::Loss;
        }
        if (!can_throw || winning.size() < 2 || winning.size() != won_children.size()) continue;
        // History C1 P C2 P ... Ck (oldest first), then the root P: king moves only, all of it reversible.
        auto run = [&](const std::vector<Position>& history) {
            engine.new_game();
            engine.set_position(p, history);
            Limits l;
            l.nodes = o.nodes;
            l.use_book = false;
            l.searchmoves.assign(legal.begin(), legal.end());
            return engine.go(l).best;
        };
        std::vector<Position> history;
        for (const Position& c : won_children) {
            if (!history.empty()) history.push_back(p);
            history.push_back(c);
        }
        const Move m = run(history);
        ++cases;
        kept += std::find(winning.begin(), winning.end(), m) != winning.end();
        // The position after the move played above occurred twice: now that move draws by threefold repetition.
        const std::size_t i = std::find(winning.begin(), winning.end(), m) - winning.begin();
        if (i == winning.size()) continue;
        history.insert(history.begin(), {won_children[i], p});
        const Move again = run(history);
        ++threefold_cases;
        threefold_avoided += again != m && std::find(winning.begin(), winning.end(), again) != winning.end();
    }
    check(cases > 20 && kept == cases, "every winning move repeats a position: a winning move is played in " +
                                           std::to_string(kept) + " of " + std::to_string(cases) + " positions");
    check(threefold_cases > 20 && threefold_avoided == threefold_cases,
          "a winning move that would be the third occurrence gives way to another winning move in " +
              std::to_string(threefold_avoided) + " of " + std::to_string(threefold_cases) + " positions");
}

void test_audit(Engines& en) {
    // Positions from the external audit that the engine drew or lost with WLD only (MTC withheld).
    for (const char* fen : {"B:WK29:BK10,K16", "B:WK32:BK1,K2,K12", "B:WK19:BK9,K28"}) {
        Position p;
        p.set_fen(fen);
        if (probe(p) != Wld::Win) {
            check(false, std::string(fen) + " is a database win");
            continue;
        }
        const Game opt = en.reference(p), g = en.play(p, true);
        check(g.result == Game::Won, std::string(fen) + ": " + result_name(g.result) + " after " +
                                         std::to_string(g.plies) + " plies (reference " + std::to_string(opt.plies) +
                                         ")");
    }
}

std::vector<int> parse_list(const std::string& s) {
    std::vector<int> v;
    for (std::size_t pos = 0; pos < s.size();) {
        std::size_t end = s.find(',', pos);
        if (end == std::string::npos) end = s.size();
        v.push_back(std::atoi(s.substr(pos, end - pos).c_str()));
        pos = end + 1;
    }
    return v;
}

}  // namespace

int main(int argc, char** argv) {
    Options o;
    const bool ctest = argc == 1;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&] { return std::string(i + 1 < argc ? argv[++i] : ""); };
        if (a == "--dir") o.dir = next();
        else if (a == "--build") o.build = std::atoi(next().c_str());
        else if (a == "--pieces") o.pieces = parse_list(next());
        else if (a == "--sample") o.sample = std::atoi(next().c_str());
        else if (a == "--nodes") o.nodes = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--cap") o.cap = std::atoi(next().c_str());
        else if (a == "--role") o.role = next();
        else if (a == "--threads") o.threads = std::atoi(next().c_str());
        else if (a == "--oracle-nodes") o.oracle_nodes = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--seed") o.seed = std::strtoull(next().c_str(), nullptr, 10);
        else if (a == "--list") o.list = true;
        else if (a == "--kind") {
            const std::string k = next();
            o.kind = k == "kings" ? KINGS_ONLY : k == "one" ? MEN_ONE_SIDE : k == "both" ? MEN_BOTH_SIDES : ANY_KIND;
        }
        else if (a == "--fen") o.fen = next();
        else {
            std::fprintf(stderr, "unknown option %s\n", a.c_str());
            return 2;
        }
    }

    const bool temp = o.dir.empty();
    if (temp)
        o.dir = (std::filesystem::temp_directory_path() /
                 ("regicide-conversion-test-" + std::to_string(std::random_device{}())))
                    .string();
    const int build = temp ? 4 : o.build;
    if (build && !(egdb::init(o.dir, 1024) && egdb::max_pieces() >= build)) {
        egdb::detail::BuildOptions b;
        b.dir = o.dir;
        b.threads = 4;
        b.mtc_pieces = build;
        b.verbose = false;
        check(egdb::detail::build_range(b, 2, build), "build 2.." + std::to_string(build) + " pieces in " + o.dir);
    } else if (!build) {
        check(egdb::init(o.dir, 1024), "load " + o.dir);
    }
    std::printf("database: %d pieces, MTC %d\n", egdb::max_pieces(), egdb::max_mtc_pieces());

    Engines en(o);
    if (!o.fen.empty()) {
        Position p;
        p.set_fen(o.fen);
        trace = true;
        const Game g = en.play(p, o.role != "lose");
        std::printf("%s after %d plies\n", result_name(g.result), g.plies);
        return 0;
    }
    if (ctest) test_audit(en);
    if (ctest) test_stays_won(o);
    if (ctest) test_repeated_wins(o);
    if (o.role == "win" || o.role == "both") measure(en, o, true);
    if (o.role == "lose" || o.role == "both") measure(en, o, false);

    if (temp) {
        std::error_code ec;
        std::filesystem::remove_all(o.dir, ec);
    }
    std::printf("%s\n", failures ? "FAILED" : "all conversion tests passed");
    return failures ? 1 : 0;
}
