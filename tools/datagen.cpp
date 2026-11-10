// Self-play data generation for training the evaluation.
//
//   datagen --games 100000 --nodes 5000 --threads 12 --out data/run1.bin [--evalfile w.bin] [--egdb DIR]
//           [--seeds seeds.txt [--seed-frac 0.5] [--seed-plies 2-4]]
//
// Each game starts with a few uniformly random plies, then both sides play with a fixed node budget.
// Every quiet position (side to move has no capture) is written with its search score and the game result.
// With --egdb, a game ends (and takes its result from the database) at the first position the database knows, and
// positions whose search score is proven (decisive) are kept, clamped and flagged (DataRecord::make_proven), instead
// of being dropped. With --seeds (one FEN per line), a share --seed-frac of the games (default 1) starts from a random
// seed position followed by --seed-plies random plies (default 2-4); the others start as usual (the start position
// plus --random-min..--random-max random plies).
// The first record of every game carries DataRecord::GAME_START. Write a new file per run (the normal case); --out
// appends only to a file whose records carry those flags, never to a file written by an older datagen. Proven
// labels follow tools/label.hpp; with --egdb, score adjudication counts only proven scores.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "datafile.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "label.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

struct Config {
    long games = 1000;
    int threads = 8;
    std::uint64_t nodes = 5000;
    int random_min = 4, random_max = 12;
    int max_opening_score = 300;
    int adjudicate_score = 900, adjudicate_plies = 6;
    int draw_reversible = 60;
    int max_plies = 300;
    std::uint64_t seed = 1;
    std::string out = "datagen.bin";
    std::string evalfile;
    int egdb_pieces = 0;
    std::vector<Position> seeds;
    double seed_frac = 1.0;
    int seed_rmin = 2, seed_rmax = 4;
};

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : "0"; };
        if (k == "--games") cfg.games = std::stol(next());
        else if (k == "--threads") cfg.threads = std::stoi(next());
        else if (k == "--nodes") cfg.nodes = std::stoull(next());
        else if (k == "--random-min") cfg.random_min = std::stoi(next());
        else if (k == "--random-max") cfg.random_max = std::stoi(next());
        else if (k == "--seed-frac") cfg.seed_frac = std::stod(next());
        else if (k == "--seed-plies") {
            const std::string s = next();
            const std::size_t d = s.find('-');
            cfg.seed_rmin = std::stoi(s.substr(0, d));
            cfg.seed_rmax = d == std::string::npos ? cfg.seed_rmin : std::stoi(s.substr(d + 1));
        }
        else if (k == "--seeds") {
            std::ifstream in(next());
            for (std::string line; std::getline(in, line);) {
                if (line.empty() || line[0] == '#') continue;
                Position p;
                if (p.set_fen(line)) cfg.seeds.push_back(p);
                else std::cerr << "bad seed FEN: " << line << '\n';
            }
            if (cfg.seeds.empty()) {
                std::cerr << "no seed positions\n";
                return 1;
            }
            std::printf("seeds: %zu positions\n", cfg.seeds.size());
        }
        else if (k == "--seed") cfg.seed = std::stoull(next());
        else if (k == "--out") cfg.out = next();
        else if (k == "--evalfile") cfg.evalfile = next();
        else if (k == "--egdb") {
            if (!egdb::init(next(), 4096)) {
                std::cerr << "cannot load egdb\n";
                return 1;
            }
            cfg.egdb_pieces = egdb::max_pieces();
        }
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }

    std::shared_ptr<const Evaluator> ev = Evaluator::builtin();
    if (!cfg.evalfile.empty() && !(ev = Evaluator::from_file(cfg.evalfile))) {
        std::cerr << "cannot load " << cfg.evalfile << '\n';
        return 1;
    }

    // Every game's first record is flagged (DataRecord::GAME_START). Appending is only safe to a file written that way:
    // a legacy file (no flag on its first record) would mix guessed and marked game boundaries.
    if (std::FILE* old = std::fopen(cfg.out.c_str(), "rb")) {
        DataRecord first;
        const bool has = std::fread(&first, sizeof first, 1, old) == 1;
        std::fclose(old);
        if (has && !first.game_start()) {
            std::cerr << cfg.out << " exists and was written without game-start flags: use a new file\n";
            return 1;
        }
    }
    std::FILE* out = std::fopen(cfg.out.c_str(), "ab");
    if (!out) {
        std::cerr << "cannot open " << cfg.out << '\n';
        return 1;
    }

    std::atomic<long> next_game{0}, positions{0}, incomplete{0}, partial_losses{0};
    std::atomic<long> ends[4] = {0, 0, 0, 0};  // no legal moves, draw rule (repetition/length), database, adjudicated
    std::atomic<long> results[3] = {0, 0, 0};  // black loss, draw, black win
    std::mutex mu;

    auto worker = [&](int tid) {
        std::mt19937_64 rng(cfg.seed * 1000003 + tid);
        Engine engine;
        engine.set_hash_mb(16);
        engine.set_evaluator(ev);
        engine.set_egdb_pieces(cfg.egdb_pieces);
        Limits limits;
        limits.nodes = cfg.nodes;
        std::vector<DataRecord> game;

        while (next_game++ < cfg.games) {
            const bool from_seed =
                !cfg.seeds.empty() && double(rng() % 1000000) < cfg.seed_frac * 1000000.0;
            Position pos = from_seed ? cfg.seeds[rng() % cfg.seeds.size()] : Position();
            std::vector<Position> history;
            bool ok = true;
            const int rmin = from_seed ? cfg.seed_rmin : cfg.random_min, rmax = from_seed ? cfg.seed_rmax : cfg.random_max;
            const int nrandom = rmin + int(rng() % (rmax - rmin + 1));
            for (int i = 0; i < nrandom && ok; ++i) {
                MoveList list;
                generate_moves(pos, list);
                if (list.empty()) ok = false;
                else {
                    history.push_back(pos);
                    pos.make_move(list[rng() % list.size()]);
                }
            }
            if (!ok) continue;
            engine.new_game();
            {
                Limits quick;
                quick.nodes = 2000;
                engine.set_position(pos, history);
                if (std::abs(engine.go(quick).sound_score()) > cfg.max_opening_score) continue;
            }

            game.clear();
            int result = 0;  // black's view
            int run = 0, last_sign = 0;
            for (int ply = 0;; ++ply) {
                MoveList list;
                generate_moves(pos, list);
                if (list.empty()) {
                    result = pos.side_to_move() == BLACK ? -1 : 1;
                    ends[0]++;
                    break;
                }
                int reps = 0;
                for (const Position& h : history) reps += h == pos;
                if (reps >= 2 || pos.reversible_plies() >= cfg.draw_reversible || ply >= cfg.max_plies) {
                    ends[1]++;
                    break;
                }
                // The database knows the exact result: label the game with it and stop.
                if (pos.piece_count() <= cfg.egdb_pieces) {
                    const egdb::Wld r =
                        egdb::probe(pos.pieces(BLACK), pos.pieces(WHITE), pos.kings(), pos.side_to_move());
                    if (r != egdb::Wld::Unknown) {
                        const int stm_result = r == egdb::Wld::Win ? 1 : r == egdb::Wld::Loss ? -1 : 0;
                        result = pos.side_to_move() == BLACK ? stm_result : -stm_result;
                        ends[2]++;
                        break;
                    }
                }

                engine.set_position(pos, history);
                const SearchResult r = engine.go(limits);
                // label_of (label.hpp): a win is proven, a loss only from a complete iteration (a partial loss is
                // written unproven with the last complete iteration's score); a search without a result is skipped.
                const auto lab = label_of(r);
                if (!lab) incomplete++;
                else if (lab->partial_loss && !lab->proven) partial_losses++;
                if (!list[0].is_capture() && lab) {
                    if (!lab->proven) game.push_back(DataRecord::make(pos, lab->score));
                    else if (cfg.egdb_pieces > 0) game.push_back(DataRecord::make_proven(pos, lab->score));
                }

                const int score = lab ? lab->score : 0;
                const int black_view = pos.side_to_move() == BLACK ? score : -score;
                // With the database, only proven scores adjudicate, so every decisive result is exact.
                const bool counts = lab && (cfg.egdb_pieces == 0 || lab->proven);
                const int sign = !counts                           ? 0
                                 : black_view >= cfg.adjudicate_score ? 1
                                 : black_view <= -cfg.adjudicate_score ? -1
                                                                       : 0;
                run = sign != 0 && sign == last_sign ? run + 1 : sign != 0;
                last_sign = sign;
                if (run >= cfg.adjudicate_plies) {
                    result = sign;
                    ends[3]++;
                    break;
                }
                history.push_back(pos);
                pos.make_move(r.best);
            }
            for (DataRecord& rec : game) rec.result = std::int8_t(result);
            if (!game.empty()) game[0].stm |= DataRecord::GAME_START;  // game boundaries are marked, not guessed
            results[result + 1]++;
            positions += long(game.size());
            {
                std::lock_guard lock(mu);
                std::fwrite(game.data(), sizeof(DataRecord), game.size(), out);
            }
            const long g = results[0] + results[1] + results[2];
            if (g % 1000 == 0) {
                std::printf("games %ld  positions %ld  black +%ld =%ld -%ld  ends: no-moves %ld draw-rule %ld "
                            "database %ld adjudicated %ld  incomplete searches %ld  partial losses %ld\n",
                            g, positions.load(), results[2].load(), results[1].load(), results[0].load(),
                            ends[0].load(), ends[1].load(), ends[2].load(), ends[3].load(), incomplete.load(),
                            partial_losses.load());
                std::fflush(stdout);
            }
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < cfg.threads; ++t) pool.emplace_back(worker, t);
    for (auto& t : pool) t.join();
    std::fclose(out);
    std::printf("done: games %ld positions %ld\n", results[0] + results[1] + results[2], positions.load());
    return 0;
}
