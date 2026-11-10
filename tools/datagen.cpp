// Self-play data generation for training the evaluation.
//
//   datagen --games 100000 --nodes 5000 --threads 12 --out data/run1.bin [--evalfile w.bin]
//
// Each game starts with a few uniformly random plies, then both sides play with a fixed node budget.
// Every quiet position (side to move has no capture) whose search score is not decisive (a proven win or loss) is
// written with that score and the game result. The result is a loss for the side without a move; a draw on a third
// occurrence, after 60 plies without a capture or man move, or after 300 plies; the database value once the piece
// count is within the database (--egdb); or a win for one side once the score has been 900 or more in its favour
// for 6 plies in a row.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
    // Stop and label the game a draw after this many plies without a capture or man move: earlier than the 79-ply
    // rule of the search and the match runner (NO_PROGRESS_PLIES in src/search.hpp) and than selfplay's 80, so long
    // king endings stop sooner. The value predates the search's rule and was not changed with it.
    int draw_reversible = 60;
    int max_plies = 300;
    std::uint64_t seed = 1;
    std::string out = "datagen.bin";
    std::string evalfile;
    int egdb_pieces = 0;
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

    std::FILE* out = std::fopen(cfg.out.c_str(), "ab");
    if (!out) {
        std::cerr << "cannot open " << cfg.out << '\n';
        return 1;
    }

    std::atomic<long> next_game{0}, positions{0};
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
            Position pos;
            std::vector<Position> history;
            bool ok = true;
            const int nrandom = cfg.random_min + int(rng() % (cfg.random_max - cfg.random_min + 1));
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
                    break;
                }
                int reps = 0;
                for (const Position& h : history) reps += h == pos;
                if (reps >= 2 || pos.reversible_plies() >= cfg.draw_reversible || ply >= cfg.max_plies) break;
                // The database knows the exact result: label the game with it and stop.
                if (pos.piece_count() <= cfg.egdb_pieces) {
                    const egdb::Wld r =
                        egdb::probe(pos.pieces(BLACK), pos.pieces(WHITE), pos.kings(), pos.side_to_move());
                    if (r != egdb::Wld::Unknown) {
                        const int stm_result = r == egdb::Wld::Win ? 1 : r == egdb::Wld::Loss ? -1 : 0;
                        result = pos.side_to_move() == BLACK ? stm_result : -stm_result;
                        break;
                    }
                }

                engine.set_position(pos, history);
                const SearchResult r = engine.go(limits);
                const Score score = r.sound_score();
                if (!list[0].is_capture() && !is_decisive(score)) game.push_back(DataRecord::make(pos, score));

                const int black_view = pos.side_to_move() == BLACK ? score : -score;
                const int sign = black_view >= cfg.adjudicate_score ? 1 : black_view <= -cfg.adjudicate_score ? -1 : 0;
                run = sign != 0 && sign == last_sign ? run + 1 : sign != 0;
                last_sign = sign;
                if (run >= cfg.adjudicate_plies) {
                    result = sign;
                    break;
                }
                history.push_back(pos);
                pos.make_move(r.best);
            }
            for (DataRecord& rec : game) rec.result = std::int8_t(result);
            results[result + 1]++;
            positions += long(game.size());
            {
                std::lock_guard lock(mu);
                std::fwrite(game.data(), sizeof(DataRecord), game.size(), out);
            }
            const long g = results[0] + results[1] + results[2];
            if (g % 1000 == 0) {
                std::printf("games %ld  positions %ld  black +%ld =%ld -%ld\n", g, positions.load(),
                            results[2].load(), results[1].load(), results[0].load());
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
