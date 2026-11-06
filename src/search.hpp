#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

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
};

struct SearchInfo {
    int depth = 0;
    int seldepth = 0;
    Score score = 0;
    std::uint64_t nodes = 0;
    std::int64_t time_ms = 0;
    int hashfull = 0;
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

    SearchParams params;

private:
    friend struct Worker;

    TranspositionTable tt_;
    std::vector<std::unique_ptr<struct Worker>> workers_;
    Position root_;
    std::vector<Key> history_;
    std::atomic<bool> stop_{false};
    std::function<bool()> stop_poll_;
    int threads_ = 1;
};

}  // namespace regicide
