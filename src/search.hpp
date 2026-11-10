#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "book.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "tt.hpp"

namespace regicide {

// A game is drawn after this many plies in a row without a capture or a man move: CheckerBoard's rule, 40 moves
// by one side and 39 by the other. The match runner applies the same count with its own code (tools/match/main.cpp,
// --no-progress 40). tools/selfplay.cpp (80 plies) and tools/datagen.cpp (60 plies) end their games by limits of
// their own, which predate this constant; see the comments there.
constexpr int NO_PROGRESS_PLIES = 79;

struct Limits {
    int depth = MAX_PLY - 1;
    std::uint64_t nodes = 0;  // 0 = unlimited
    int movetime_ms = 0;      // fixed time for this move
    int target_ms = 0;        // average time per move; individual moves may use less or more
    int time_ms = 0;          // remaining clock of the side to move (0 = no clock)
    int inc_ms = 0;
    int moves_to_go = 0;  // 0 = sudden death
    bool infinite = false;
    bool use_book = true;  // play a book move without searching when the engine has a book
    // Analysis: search only these root moves (legal ones; empty = all). Skips the book and the avoid list, and
    // searches a single move to the full limits instead of returning it at once. Exception: a root the database
    // decides with MTC (Engine::egdb_root_move) is answered from the database among these moves, without a search,
    // unless `infinite` is set.
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
    // The best score known when the search ended. If the last iteration was cut short (`partial`), this comes from
    // that iteration: it can be an aspiration bound instead of a score, and it covers only the root moves searched
    // so far, so a losing score then says that those moves lose, not that the position is lost.
    Score score = 0;
    int depth = 0;  // last completed iteration
    Score completed_score = -SCORE_INF;  // its score; -SCORE_INF if no iteration was completed
    bool partial = false;
    // The score as a statement about the position: a losing score from a partial iteration only says that the
    // moves searched so far lose, so the last completed score stands in for it (0 if no iteration was completed).
    Score sound_score() const {
        if (!partial || score > -SCORE_KNOWN_WIN) return score;
        return completed_score != -SCORE_INF ? completed_score : SCORE_DRAW;
    }
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
    // Polled by the main search thread; returning true aborts the search (e.g. a GUI "play now").
    void set_stop_poll(std::function<bool()> poll) { stop_poll_ = std::move(poll); }
    // Clears the hash table: it holds static evals and scores of the previous evaluator.
    void set_evaluator(std::shared_ptr<const Evaluator> e) {
        eval_ = std::move(e);
        tt_.clear();
    }
    // Probe the (process-wide, already initialised) endgame database in positions with at most n pieces; 0 = off.
    // A change clears the hash table: scores and evals of database positions depend on it.
    void set_egdb_pieces(int n) {
        if (n != egdb_pieces_) tt_.clear();
        egdb_pieces_ = n;
    }
    int egdb_pieces() const { return egdb_pieces_; }
    // Use moves-to-conversion only in positions with at most n pieces (default: wherever the database has it).
    // Lets tests stand in small generated tables for the real database's WLD-only range.
    void set_egdb_mtc_pieces(int n) {
        if (n != egdb_mtc_pieces_) tt_.clear();
        egdb_mtc_pieces_ = n;
    }
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
    const TranspositionTable& hash() const { return tt_; }  // for tests and diagnostics

    SearchParams params;

private:
    friend struct Worker;

    std::optional<std::pair<Move, Score>> egdb_root_move(const Position& root, const std::vector<Move>& list) const;

    TranspositionTable tt_;
    // Late-move reductions by depth and move number, filled from `params` by go(). Per engine: engines with
    // different parameters search at the same time in one process (selfplay).
    int lmr_[64][64] = {};
    std::vector<std::unique_ptr<struct Worker>> workers_;
    Position root_;
    std::vector<Key> history_;
    std::atomic<bool> stop_{false};
    std::function<bool()> stop_poll_;
    std::shared_ptr<const Evaluator> eval_ = Evaluator::builtin();
    std::shared_ptr<const Book> book_;
    std::shared_ptr<const Book> avoid_;
    int egdb_pieces_ = 0;
    int egdb_mtc_pieces_ = 32;
    int conversion_ = 0;  // this search converts a database win (+1) or defends a database loss (-1); see go()
    int book_margin_ = 0;
    std::uint64_t book_seed_ = 0x1234567;
    int threads_ = 1;
};

}  // namespace regicide
