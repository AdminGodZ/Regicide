// In-process A/B match between two Regicide configurations.
//
//   selfplay --a "lmr_mult=50" --b "" --nodes 20000 --threads 12 --pairs 300
//
// --threads is the number of games played in parallel; --tha/--thb are search threads per engine.
// --avoida/--avoidb <file|builtin> give an engine the avoid list of a book file (default: none).
// --book <file> replaces the 3-ply set with the positions of a text file (first token of each line: a FEN).
//
// Every distinct position after 3 plies from the start is an opening; each is played twice with colours
// reversed. Reports W/D/L for A, and Elo with a 95% interval from the pentanomial distribution of game pairs.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

struct Config {
    std::string a, b;
    std::string eval_a, eval_b;  // weight files; empty = built-in, "hce" = hand-crafted
    std::string avoid_a, avoid_b;  // avoid lists (book files); empty = none, "builtin" = the embedded book's
    Limits limits;
    int threads = 8;
    int pairs = 0;  // 0 = every opening once
    std::size_t hash_mb = 16;
    int adjudicate_score = 600;
    int adjudicate_plies = 8;
    // Draw after this many plies without a capture or man move. One ply later than the 79-ply rule the search and
    // the match runner apply (NO_PROGRESS_PLIES in src/search.hpp, --no-progress in tools/match/main.cpp): the value
    // predates that rule and was not changed with it. tools/datagen.cpp uses 60. The match runner's ply cap is 1000.
    int draw_reversible = 80;
    int max_plies = 400;
    int opening_plies = 3;
    std::string book;  // opening positions file (FEN per line); empty = every position after opening_plies
    int egdb_pieces = 0;  // engines probe the database and games end on a database result
    int threads_a = 1, threads_b = 1;  // search threads per engine (Lazy SMP)
};

std::vector<Position> openings_at(int plies) {
    std::vector<Position> out;
    std::set<Key> seen;
    auto rec = [&](auto& self, const Position& p, int depth) -> void {
        if (depth == 0) {
            if (seen.insert(p.key()).second) out.push_back(p);
            return;
        }
        MoveList list;
        generate_moves(p, list);
        for (const Move& m : list) {
            Position c = p;
            c.make_move(m);
            self(self, c, depth - 1);
        }
    };
    rec(rec, Position(), plies);
    return out;
}

// Time and depth per engine side (A / B).
struct SideStats {
    std::atomic<long long> ms{0}, moves{0}, depth{0};
};
SideStats stats_a, stats_b;

// Plays one game; returns black's score (1, 0.5, 0).
double play_game(const Position& start, Engine& black, Engine& white, const Config& cfg, SideStats& sb,
                 SideStats& sw) {
    Position pos = start;
    std::vector<Position> history;
    black.new_game();
    white.new_game();
    int decisive_run = 0, last_sign = 0;

    for (int ply = 0; ply < cfg.max_plies; ++ply) {
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) return pos.side_to_move() == BLACK ? 0.0 : 1.0;
        if (pos.reversible_plies() >= cfg.draw_reversible) return 0.5;
        if (pos.piece_count() <= cfg.egdb_pieces) {
            const egdb::Wld r = egdb::probe(pos.pieces(BLACK), pos.pieces(WHITE), pos.kings(), pos.side_to_move());
            if (r == egdb::Wld::Draw) return 0.5;
            if (r != egdb::Wld::Unknown) return (r == egdb::Wld::Win) == (pos.side_to_move() == BLACK) ? 1.0 : 0.0;
        }
        int reps = 0;
        for (const Position& h : history) reps += h == pos;
        if (reps >= 2) return 0.5;

        Engine& e = pos.side_to_move() == BLACK ? black : white;
        e.set_position(pos, history);
        SearchResult r = e.go(cfg.limits);
        SideStats& st = pos.side_to_move() == BLACK ? sb : sw;
        st.ms += r.time_ms;
        st.moves += 1;
        st.depth += r.depth;

        // Adjudicate when both sides agree the game is decided for several plies.
        const int score = r.sound_score();
        const int black_view = pos.side_to_move() == BLACK ? score : -score;
        const int sign = black_view >= cfg.adjudicate_score ? 1 : black_view <= -cfg.adjudicate_score ? -1 : 0;
        decisive_run = sign != 0 && sign == last_sign ? decisive_run + 1 : sign != 0;
        last_sign = sign;
        if (cfg.adjudicate_score > 0 && decisive_run >= cfg.adjudicate_plies) return sign > 0 ? 1.0 : 0.0;

        history.push_back(pos);
        pos.make_move(r.best);
    }
    return 0.5;
}

// The same estimate as the match runner's (tools/match/stats.cpp: pentanomial mean, population variance of the pair
// scores, interval endpoints mapped through the Elo formula), written separately because selfplay links only
// regicide_core. The pool.py scripts under experiments/ use the n - 1 variance and a delta-method interval instead.
void report(const char* tag, const std::vector<int>& penta, int w, int d, int l) {
    const int n = w + d + l, pairs = penta[0] + penta[1] + penta[2] + penta[3] + penta[4];
    if (!n || !pairs) return;
    const double score = (w + 0.5 * d) / n;
    double mean = 0, var = 0;
    for (int i = 0; i < 5; ++i) mean += penta[i] * (i / 4.0);
    mean /= pairs;
    for (int i = 0; i < 5; ++i) var += penta[i] * std::pow(i / 4.0 - mean, 2);
    var /= pairs;
    auto elo = [](double s) {
        s = std::clamp(s, 1e-6, 1 - 1e-6);
        return -400.0 * std::log10(1.0 / s - 1.0);
    };
    const double se = std::sqrt(var / pairs);
    const double lo = elo(mean - 1.96 * se), hi = elo(mean + 1.96 * se);
    std::printf("%s games %d  A: +%d =%d -%d  score %.4f  draws %.1f%%  elo %+.1f [%+.1f, %+.1f]  penta [%d %d %d %d %d]\n",
                tag, n, w, d, l, score, 100.0 * d / n, elo(score), lo, hi, penta[0], penta[1], penta[2], penta[3],
                penta[4]);
    std::fflush(stdout);
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    cfg.limits.nodes = 20000;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--a") cfg.a = next();
        else if (k == "--b") cfg.b = next();
        else if (k == "--evala") cfg.eval_a = next();
        else if (k == "--evalb") cfg.eval_b = next();
        else if (k == "--avoida") cfg.avoid_a = next();
        else if (k == "--avoidb") cfg.avoid_b = next();
        else if (k == "--nodes") cfg.limits.nodes = std::stoull(next());
        else if (k == "--depth") cfg.limits.depth = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--movetime") cfg.limits.movetime_ms = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--target") cfg.limits.target_ms = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--threads") cfg.threads = std::stoi(next());
        else if (k == "--pairs") cfg.pairs = std::stoi(next());
        else if (k == "--hash") cfg.hash_mb = std::stoull(next());
        else if (k == "--plies") cfg.opening_plies = std::stoi(next());
        else if (k == "--book") cfg.book = next();
        else if (k == "--tha") cfg.threads_a = std::stoi(next());
        else if (k == "--thb") cfg.threads_b = std::stoi(next());
        else if (k == "--egdb") {
            if (!egdb::init(next(), 4096)) {
                std::cerr << "cannot load egdb\n";
                return 1;
            }
            cfg.egdb_pieces = egdb::max_pieces();
        }
        else if (k == "--adjudicate") cfg.adjudicate_score = std::stoi(next());
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }

    std::vector<Position> openings;
    if (cfg.book.empty()) {
        openings = openings_at(cfg.opening_plies);
    } else {
        std::FILE* f = std::fopen(cfg.book.c_str(), "r");
        if (!f) {
            std::cerr << "cannot open " << cfg.book << '\n';
            return 1;
        }
        char line[512];
        while (std::fgets(line, sizeof line, f)) {
            std::string fen = line;
            fen = fen.substr(0, fen.find_first_of(" \t\r\n"));
            if (fen.empty() || fen[0] == '#') continue;
            Position p;
            if (!p.set_fen(fen)) {
                std::cerr << "bad FEN " << fen << '\n';
                return 1;
            }
            openings.push_back(p);
        }
        std::fclose(f);
        if (openings.empty()) return 1;
    }
    // Spread a partial run across the whole opening set.
    std::uint64_t shuffle_state = 0x9E3779B97F4A7C15ull;
    for (std::size_t i = openings.size(); i > 1; --i) {
        shuffle_state = shuffle_state * 6364136223846793005ull + 1442695040888963407ull;
        std::swap(openings[i - 1], openings[(shuffle_state >> 33) % i]);
    }
    const int pairs = cfg.pairs ? cfg.pairs : int(openings.size());
    std::printf("openings %zu, pairs %d, A=\"%s\" B=\"%s\"\n", openings.size(), pairs, cfg.a.c_str(),
                cfg.b.c_str());

    auto load_eval = [](const std::string& spec) {
        if (spec.empty()) return Evaluator::builtin();
        if (spec == "hce") return Evaluator::hce();
        auto e = Evaluator::from_file(spec);
        if (!e) {
            std::cerr << "cannot load " << spec << '\n';
            std::exit(1);
        }
        return e;
    };
    const auto eval_a = load_eval(cfg.eval_a), eval_b = load_eval(cfg.eval_b);
    auto load_avoid = [](const std::string& spec) -> std::shared_ptr<const Book> {
        if (spec.empty()) return nullptr;
        if (spec == "builtin") return Book::builtin();
        auto b = std::make_shared<Book>();
        if (!b->load(spec)) {
            std::cerr << "cannot load " << spec << '\n';
            std::exit(1);
        }
        std::printf("avoid list %s: %zu entries\n", spec.c_str(), b->avoid_size());
        return b;
    };
    const auto avoid_a = load_avoid(cfg.avoid_a), avoid_b = load_avoid(cfg.avoid_b);

    std::atomic<int> next_pair{0};
    std::mutex mu;
    std::vector<int> penta(5, 0);
    int w = 0, d = 0, l = 0, done = 0;

    auto worker = [&] {
        Engine ea, eb;
        ea.set_hash_mb(cfg.hash_mb);
        eb.set_hash_mb(cfg.hash_mb);
        ea.set_evaluator(eval_a);
        eb.set_evaluator(eval_b);
        ea.set_avoid(avoid_a);
        eb.set_avoid(avoid_b);
        ea.set_egdb_pieces(cfg.egdb_pieces);
        eb.set_egdb_pieces(cfg.egdb_pieces);
        ea.set_threads(cfg.threads_a);
        eb.set_threads(cfg.threads_b);
        if (!ea.params.set_all(cfg.a) || !eb.params.set_all(cfg.b)) {
            std::cerr << "bad parameter spec\n";
            std::exit(2);
        }
        for (int i; (i = next_pair++) < pairs;) {
            const Position& open = openings[i % openings.size()];
            const double g1 = play_game(open, ea, eb, cfg, stats_a, stats_b);        // A is black
            const double g2 = 1.0 - play_game(open, eb, ea, cfg, stats_b, stats_a);  // A is white
            std::lock_guard lock(mu);
            for (double g : {g1, g2}) (g == 1.0 ? w : g == 0.0 ? l : d)++;
            penta[int(std::lround((g1 + g2) * 2))]++;
            if (++done % 20 == 0 || done == pairs) report(done == pairs ? "final" : "     ", penta, w, d, l);
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < cfg.threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    auto side = [](const char* tag, const SideStats& s) {
        const double n = std::max(1LL, s.moves.load());
        std::printf("%s: moves %lld  mean ms %.1f  mean depth %.2f\n", tag, s.moves.load(), s.ms.load() / n,
                    s.depth.load() / n);
    };
    side("time A", stats_a);
    side("time B", stats_b);
    return 0;
}
