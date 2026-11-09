#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "book.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "tt.hpp"

namespace regicide {

struct Limits {
    int depth = MAX_PLY - 1;
    std::uint64_t nodes = 0;  // 0 = unlimited
    int movetime_ms = 0;      // fixed time for this move
    int target_ms = 0;        // average time per move; individual moves may use less or more
    int time_ms = 0;          // remaining clock of the side to move (0 = no clock)
    int inc_ms = 0;
    int moves_to_go = 0;  // 0 = sudden death
    bool infinite = false;
    // Think on the opponent's time: search without limits until Engine::ponderhit() supplies them.
    bool ponder = false;
    bool use_book = true;  // play a book move without searching when the engine has a book
    // Analysis: search only these root moves (legal ones; empty = all). Skips the book and the avoid list, and
    // searches a single move to the full limits instead of returning it at once.
    std::vector<Move> searchmoves;
};

struct SearchInfo {
    int depth = 0;
    int seldepth = 0;
    Score score = 0;
    std::uint64_t nodes = 0;
    std::int64_t time_ms = 0;
    int hashfull = 0;
    std::uint64_t tbhits = 0;
    std::vector<Move> pv;
};

struct SearchResult {
    Move best;
    Score score = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
    std::int64_t time_ms = 0;
    std::vector<Move> pv;
};

// Tunable search parameters (exposed for automated tuning).
struct SearchParams {
    int aspiration_delta = 12;
    int rfp_margin = 70;        // reverse futility margin per ply
    int rfp_max_depth = 4;
    int lmr_base = 60;          // reduction = (base + log(d) * log(i) * mult) / 100
    int lmr_mult = 45;
    int lmr_min_depth = 3;
    int threat_qext = 1;        // extra plies allowed at the horizon when the opponent threatens a capture
    int single_reply_ext = 1;
    int etc_min_depth = 4;      // enhanced transposition cutoff from this depth (99 = off)
    int probcut_depth = 99;     // ProbCut from this depth (99 = off)
    int probcut_margin = 80;
    int probcut_reduction = 4;
    int lmp_depth = 3;          // late move pruning up to this depth (0 = off)
    int lmp_base = 3;
    int fut_depth = 3;          // per-move futility pruning up to this depth (0 = off)
    int fut_margin = 60;
    int cont_hist = 1;          // use counter-move continuation history in ordering
    int se_depth = 4;           // singular extensions from this depth (99 = off); tune1: se4 vs se8 +13.5 at 300 ms
    int se_margin = 20;         // singular beta = tt score - se_margin * depth / 8 (tune1: 20 vs 40 +10.9 at 300 ms)
    int razor_depth = 0;        // razoring up to this depth (0 = off)
    int razor_margin = 150;
    int tm_soft = 60;           // CheckerBoard time: soft limit (no new iteration) in % of maxtime
    int tm_hard = 250;          // ... and hard limit (abort) in % of maxtime
    int tm_drop = 0;            // score drop (cp) vs the previous iteration that buys extra time (0 = off)
    int tm_drop_factor = 200;   // soft-limit multiplier in % while the score is dropping
    int tm_open_pieces = 0;     // with at least this many pieces on the board ... (0 = off)
    int tm_open_factor = 150;   // ... CheckerBoard soft/hard limits are scaled by this % (spend the bank early)

    // Sets a parameter by name; returns false for an unknown name.
    bool set(const std::string& name, int value);
    // Applies "name=value,name=value" pairs; returns false if any name is unknown.
    bool set_all(const std::string& spec);
};

class Engine {
public:
    Engine();
    ~Engine();

    void set_threads(int n);
    void set_hash_mb(std::size_t mb);
    void new_game();  // clears the transposition table and history

    // `history` holds the game positions before `root`, oldest first (for repetition draws).
    void set_position(const Position& root, const std::vector<Position>& history = {});

    using InfoFn = std::function<void(const SearchInfo&)>;
    SearchResult go(const Limits& limits, InfoFn on_info = {});
    void stop() { stop_.store(true, std::memory_order_relaxed); }

    // Pondering. go() with Limits::ponder searches without limits; ponderhit() (from another thread) turns it into a
    // normal search with the time and node limits of `limits`, its clock starting at the hit. If the ponder search
    // has already gone further than that search would (by the soft limit and the best move's stability), it stops at
    // once. expect_ponder() announces the next ponder search, so a hit that comes before that search has started
    // still applies to it. Returns false if no ponder search is running or announced (it has finished).
    void expect_ponder();
    bool ponderhit(const Limits& limits);
    bool pondering() const { return pondering_.load(std::memory_order_acquire); }
    // Polled by the main search thread; returning true aborts the search (e.g. a GUI "play now").
    void set_stop_poll(std::function<bool()> poll) { stop_poll_ = std::move(poll); }
    void set_evaluator(std::shared_ptr<const Evaluator> e) { eval_ = std::move(e); }
    // Probe the (process-wide, already initialised) endgame database in positions with at most n pieces; 0 = off.
    void set_egdb_pieces(int n) { egdb_pieces_ = n; }
    int egdb_pieces() const { return egdb_pieces_; }
    // nullptr disables the book. `margin` > 0 picks randomly among book moves within margin of the best.
    void set_book(std::shared_ptr<const Book> b, int margin = 0) {
        book_ = std::move(b);
        book_margin_ = margin;
    }
    // Avoid list (Book::avoid_entries): root moves on it are never played unless every legal move is on it. The
    // book probe skips them too. nullptr disables it. Positions decided by the endgame database ignore it.
    void set_avoid(std::shared_ptr<const Book> a) { avoid_ = std::move(a); }
    const Book* avoid() const { return avoid_.get(); }
    const Evaluator& evaluator() const { return *eval_; }

    SearchParams params;

private:
    friend struct Worker;

    std::optional<std::pair<Move, Score>> egdb_root_move(const Position& root, const std::vector<Move>& list) const;
    void set_time(struct TimeControl& tc, const Limits& limits, std::chrono::steady_clock::time_point start) const;
    // Called by the main thread after each completed iteration of a ponder search: records it and returns true
    // while the search is still pondering.
    bool ponder_iteration(std::int64_t elapsed_ms, double factor, bool single);

    TranspositionTable tt_;
    std::vector<std::unique_ptr<struct Worker>> workers_;
    Position root_;
    std::vector<Key> history_;
    std::atomic<bool> stop_{false};
    std::function<bool()> stop_poll_;
    // Pondering state, guarded by tc_mutex_ (pondering_ is also read without it, by check_limits).
    std::mutex tc_mutex_;
    struct TimeControl* tc_ = nullptr;  // time control of the running search
    std::atomic<bool> pondering_{false};
    bool ponder_expected_ = false;
    bool hit_pending_ = false;  // ponderhit() came before the ponder search started
    Limits hit_limits_;
    std::chrono::steady_clock::time_point hit_time_;
    struct {
        std::int64_t elapsed_ms = -1;  // when the last ponder iteration completed (-1 = none yet)
        double factor = 1;             // soft-limit factor after it
        bool single = false;           // one root move
    } ponder_iter_;
    std::shared_ptr<const Evaluator> eval_ = Evaluator::builtin();
    std::shared_ptr<const Book> book_;
    std::shared_ptr<const Book> avoid_;
    int egdb_pieces_ = 0;
    int book_margin_ = 0;
    std::uint64_t book_seed_ = 0x1234567;
    int threads_ = 1;
};

}  // namespace regicide
