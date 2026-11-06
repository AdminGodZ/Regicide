#include "search.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

#include "movegen.hpp"

namespace regicide {

namespace {

using Clock = std::chrono::steady_clock;

std::int64_t elapsed_ms(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}

// Proven-win scores are stored relative to the node so they stay valid at other plies.
Score score_to_tt(Score s, int ply) { return s >= SCORE_KNOWN_WIN ? s + ply : s <= -SCORE_KNOWN_WIN ? s - ply : s; }
Score score_from_tt(Score s, int ply) { return s >= SCORE_KNOWN_WIN ? s - ply : s <= -SCORE_KNOWN_WIN ? s + ply : s; }

TTMove encode_move(const MoveList& list, std::size_t idx) {
    const Move& m = list[idx];
    int disc = 0;
    for (std::size_t i = 0; i < idx; ++i) disc += list[i].from == m.from && list[i].to == m.to;
    return TTMove::make(m.from, m.to, std::min(disc, 3));
}

int find_move(const MoveList& list, TTMove tm) {
    if (!tm.valid()) return -1;
    int disc = tm.disc();
    for (std::size_t i = 0; i < list.size(); ++i)
        if (list[i].from == tm.from() && list[i].to == tm.to() && disc-- == 0) return int(i);
    return -1;
}

bool is_promotion(const Position& pos, const Move& m) {
    return !(pos.kings() & bit(m.from)) && (bit(m.to) & promo_row(pos.side_to_move()));
}

// Lazy SMP: helper threads skip some depths so they search different parts of the tree.
constexpr int SKIP_SIZE[] = {1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
constexpr int SKIP_PHASE[] = {0, 1, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7};

}  // namespace

struct RootMove {
    Move move;
    Score score = -SCORE_INF;
    Score prev_score = -SCORE_INF;
    std::vector<Move> pv;
};

struct TimeControl {
    Clock::time_point start;
    std::int64_t soft_ms = 0;  // don't start a new iteration after this
    std::int64_t hard_ms = 0;  // abort the search at this point (0 = no limit)
    std::uint64_t node_limit = 0;
};

struct Worker {
    Worker(Engine& e, int id) : eng(e), tt(e.tt_), id(id) { clear(); }

    void clear() {
        std::memset(history, 0, sizeof(history));
        std::memset(killers, 0, sizeof(killers));
    }

    void setup(const Position& root, const std::vector<Key>& game_keys, const std::vector<Move>& root_moves);
    void iterate(const Limits& limits, TimeControl& tc, const Engine::InfoFn* on_info);
    Score search(Position& pos, Score alpha, Score beta, int depth, int ply, bool cut_node, int qext);

    bool stopped() const { return eng.stop_.load(std::memory_order_relaxed); }
    void check_limits();
    bool is_repetition(const Position& pos, int ply) const;
    void update_quiet_stats(Color us, const Move& best, const Move* tried, int ntried, int depth, int ply);
    SearchInfo make_info(int depth, Score score) const;

    Engine& eng;
    TranspositionTable& tt;
    const int id;
    SearchParams P;
    TimeControl* tc = nullptr;

    Position root;
    std::vector<RootMove> roots;
    std::vector<Key> keys;  // game history keys followed by the current search path
    std::size_t hist_len = 0;

    std::int16_t history[2][32][32];
    Move killers[MAX_PLY + 8][2];
    Move pv[MAX_PLY + 8][MAX_PLY + 8];
    int pv_len[MAX_PLY + 8];

    std::atomic<std::uint64_t> nodes{0};
    int seldepth = 0;
    int root_depth = 0;
    int completed_depth = 0;
};

namespace {

int LMR[64][64];

void init_lmr(const SearchParams& P) {
    for (int d = 1; d < 64; ++d)
        for (int i = 1; i < 64; ++i)
            LMR[d][i] = int((P.lmr_base + std::log(double(d)) * std::log(double(i)) * P.lmr_mult) / 100.0);
}

}  // namespace

void Worker::setup(const Position& r, const std::vector<Key>& game_keys, const std::vector<Move>& root_moves) {
    root = r;
    hist_len = game_keys.size();
    keys.assign(hist_len + MAX_PLY + 8, 0);
    std::copy(game_keys.begin(), game_keys.end(), keys.begin());
    roots.clear();
    for (const Move& m : root_moves) roots.push_back({m, -SCORE_INF, -SCORE_INF, {m}});
    nodes = 0;
    seldepth = 0;
    completed_depth = 0;
    P = eng.params;
}

bool Worker::is_repetition(const Position& pos, int ply) const {
    const long idx = long(hist_len) + ply;
    const long stop = std::max(0L, idx - pos.reversible_plies());
    for (long i = idx - 2; i >= stop; i -= 2)
        if (keys[i] == pos.key()) return true;
    return false;
}

void Worker::check_limits() {
    if (!tc) return;
    if (tc->hard_ms && elapsed_ms(tc->start) >= tc->hard_ms) eng.stop();
    if (tc->node_limit && nodes.load(std::memory_order_relaxed) * eng.threads_ >= tc->node_limit) eng.stop();
}

void Worker::update_quiet_stats(Color us, const Move& best, const Move* tried, int ntried, int depth, int ply) {
    const int bonus = std::min(depth * depth * 8, 4000);
    auto adjust = [&](const Move& m, int b) {
        std::int16_t& h = history[us][m.from][m.to];
        h = std::int16_t(h + b - h * std::abs(b) / 16384);
    };
    adjust(best, bonus);
    for (int i = 0; i < ntried; ++i) adjust(tried[i], -bonus);
    if (killers[ply][0] != best) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = best;
    }
}

Score Worker::search(Position& pos, Score alpha, Score beta, int depth, int ply, bool cut_node, int qext) {
    const bool pv_node = beta - alpha > 1;
    const bool is_root = ply == 0;
    const Color us = pos.side_to_move();

    if ((nodes.fetch_add(1, std::memory_order_relaxed) & 1023) == 0 && id == 0) check_limits();
    if (stopped()) return 0;
    pv_len[ply] = ply;
    seldepth = std::max(seldepth, ply);
    keys[hist_len + ply] = pos.key();

    if (!is_root) {
        if (is_repetition(pos, ply)) return SCORE_DRAW;
        // Mate distance pruning.
        alpha = std::max(alpha, -SCORE_WIN + ply);
        beta = std::min(beta, SCORE_WIN - ply - 1);
        if (alpha >= beta) return alpha;
        if (ply >= MAX_PLY - 2) return evaluate(pos);
    }

    TTData tte;
    const bool tt_hit = tt.probe(pos.key(), tte);
    const Score tt_score = tt_hit ? score_from_tt(tte.score, ply) : 0;
    if (!pv_node && tt_hit && tte.depth >= depth &&
        (tte.bound & (tt_score >= beta ? BOUND_LOWER : BOUND_UPPER)))
        return tt_score;

    MoveList moves;
    if (is_root)
        for (const RootMove& rm : roots) moves.push(rm.move);
    else
        generate_moves(pos, moves);
    if (moves.empty()) return -SCORE_WIN + ply;

    const bool forced_capture = moves[0].is_capture();
    Score eval = tt_hit ? Score(tte.eval) : evaluate(pos);

    if (depth <= 0 && !forced_capture) {
        // Horizon. If the opponent threatens a capture the static eval can't be trusted: look one ply further.
        if (qext > 0 && has_capture(pos, ~us)) {
            --qext;
            depth = 1;
        } else {
            return eval;
        }
    }

    // Reverse futility pruning: far above beta at low depth, assume a quiet move holds it.
    if (!pv_node && !forced_capture && depth > 0 && depth <= P.rfp_max_depth && !is_decisive(beta) &&
        eval - P.rfp_margin * depth >= beta && !has_capture(pos, ~us))
        return eval;

    // Internal iterative reduction: no hash move at high depth means this node is probably unimportant.
    if (depth >= 6 && !(tt_hit && tte.move.valid())) --depth;

    const int ext = depth > 0 && moves.size() == 1 && P.single_reply_ext ? 1 : 0;

    // Move ordering.
    int order[MoveList::CAPACITY];
    const int tt_idx = is_root ? 0 : find_move(moves, tt_hit ? tte.move : TTMove{});
    for (std::size_t i = 0; i < moves.size(); ++i) {
        const Move& m = moves[i];
        int s;
        if (is_root)
            s = -int(i);  // keep the order from the previous iteration
        else if (int(i) == tt_idx)
            s = 1 << 30;
        else if (m.is_capture())
            s = (1 << 24) + 1024 * popcount(m.captures) + 256 * popcount(m.captures & pos.kings());
        else if (is_promotion(pos, m))
            s = 1 << 23;
        else if (m == killers[ply][0])
            s = (1 << 22) + 1;
        else if (m == killers[ply][1])
            s = 1 << 22;
        else
            s = history[us][m.from][m.to];
        order[i] = s;
    }

    Score best = -SCORE_INF;
    int best_idx = -1;
    Move quiets_tried[MoveList::CAPACITY];
    int nquiets = 0;

    for (std::size_t n = 0; n < moves.size(); ++n) {
        // Selection sort: bring the best-ordered remaining move to position n.
        std::size_t bi = n;
        for (std::size_t j = n + 1; j < moves.size(); ++j)
            if (order[j] > order[bi]) bi = j;
        std::swap(moves[n], moves[bi]);
        std::swap(order[n], order[bi]);
        const Move m = moves[n];
        const bool quiet = !m.is_capture() && !is_promotion(pos, m);

        Position child = pos;
        child.make_move(m);
        tt.prefetch(child.key());
        const int new_depth = depth - 1 + ext;

        Score s;
        if (n == 0) {
            s = -search(child, -beta, -alpha, new_depth, ply + 1, false, qext);
        } else {
            int r = 0;
            if (depth >= P.lmr_min_depth && quiet) {
                r = LMR[std::min(depth, 63)][std::min<int>(int(n), 63)];
                r += cut_node;
                r -= pv_node;
                r -= history[us][m.from][m.to] / 6000;
                r = std::clamp(r, 0, std::max(0, new_depth - 1));
            }
            s = -search(child, -alpha - 1, -alpha, new_depth - r, ply + 1, true, qext);
            if (s > alpha && r > 0) s = -search(child, -alpha - 1, -alpha, new_depth, ply + 1, !cut_node, qext);
            if (s > alpha && s < beta) s = -search(child, -beta, -alpha, new_depth, ply + 1, false, qext);
        }
        if (stopped()) return 0;

        if (is_root) {
            RootMove& rm = *std::find_if(roots.begin(), roots.end(), [&](const RootMove& r) { return r.move == m; });
            if (n == 0 || s > alpha) {
                rm.score = s;
                rm.pv.assign(1, m);
                rm.pv.insert(rm.pv.end(), pv[ply + 1] + ply + 1, pv[ply + 1] + pv_len[ply + 1]);
            } else {
                rm.score = -SCORE_INF;
            }
        }

        if (s > best) {
            best = s;
            if (s > alpha) {
                best_idx = int(n);
                pv[ply][ply] = m;
                for (int k = ply + 1; k < pv_len[ply + 1]; ++k) pv[ply][k] = pv[ply + 1][k];
                pv_len[ply] = std::max(pv_len[ply + 1], ply + 1);
                if (s >= beta) {
                    if (quiet) update_quiet_stats(us, m, quiets_tried, nquiets, depth, ply);
                    break;
                }
                alpha = s;
            }
        }
        if (quiet) quiets_tried[nquiets++] = m;
    }

    const Bound bound = best >= beta ? BOUND_LOWER : best_idx >= 0 ? BOUND_EXACT : BOUND_UPPER;
    // The moves array was permuted by the selection sort, so encode against the same array.
    tt.store(pos.key(), score_to_tt(best, ply), eval, std::max(depth, -8), bound,
             best_idx >= 0 ? encode_move(moves, std::size_t(best_idx)) : TTMove{});
    return best;
}

SearchInfo Worker::make_info(int depth, Score score) const {
    SearchInfo info;
    info.depth = depth;
    info.seldepth = seldepth;
    info.score = score;
    for (const auto& w : eng.workers_) info.nodes += w->nodes.load(std::memory_order_relaxed);
    info.time_ms = elapsed_ms(tc->start);
    info.hashfull = tt.hashfull();
    info.pv = roots[0].pv;
    return info;
}

void Worker::iterate(const Limits& limits, TimeControl& t, const Engine::InfoFn* on_info) {
    tc = &t;
    const bool main = id == 0;
    Move last_best = roots[0].move;
    int stability = 0;

    for (root_depth = 1; root_depth <= limits.depth; ++root_depth) {
        if (!main && id < 20 && ((root_depth + SKIP_PHASE[id]) / SKIP_SIZE[id]) % 2) continue;

        for (RootMove& rm : roots) {
            rm.prev_score = rm.score;
            rm.score = -SCORE_INF;
        }
        seldepth = 0;

        Score prev = roots[0].prev_score;
        Score delta = P.aspiration_delta;
        Score alpha = -SCORE_INF, beta = SCORE_INF;
        if (root_depth >= 4 && !is_decisive(prev)) {
            alpha = std::max(prev - delta, -SCORE_INF);
            beta = std::min(prev + delta, SCORE_INF);
        }

        Score score;
        while (true) {
            score = search(root, alpha, beta, root_depth, 0, false, P.threat_qext);
            std::stable_sort(roots.begin(), roots.end(),
                             [](const RootMove& a, const RootMove& b) { return a.score > b.score; });
            if (stopped()) break;
            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha = std::max(score - delta, -SCORE_INF);
            } else if (score >= beta) {
                beta = std::min(score + delta, SCORE_INF);
            } else {
                break;
            }
            delta += delta / 2 + 5;
        }
        if (stopped()) break;
        completed_depth = root_depth;

        if (!main) continue;
        if (on_info && *on_info) (*on_info)(make_info(root_depth, score));

        if (roots[0].move == last_best)
            ++stability;
        else
            stability = 0;
        last_best = roots[0].move;

        if (limits.infinite) continue;
        if (roots.size() == 1 && (t.soft_ms || t.hard_ms)) break;
        if (t.soft_ms) {
            // A stable best move lets us stop early; a fresh change of mind buys extra time.
            const double factor = stability >= 6 ? 0.5 : stability >= 3 ? 0.8 : stability == 0 ? 1.6 : 1.0;
            if (elapsed_ms(t.start) > std::int64_t(t.soft_ms * factor)) break;
        }
        if (is_decisive(score) && SCORE_WIN - std::abs(score) < root_depth && std::abs(score) > SCORE_MATE_BOUND)
            break;  // found a forced win/loss shorter than the search depth
    }
    if (main) eng.stop();
}

Engine::Engine() {
    tt_.resize(64);
    set_threads(1);
}

Engine::~Engine() = default;

void Engine::set_threads(int n) {
    threads_ = std::max(1, n);
    workers_.clear();
    for (int i = 0; i < threads_; ++i) workers_.push_back(std::make_unique<Worker>(*this, i));
}

void Engine::set_hash_mb(std::size_t mb) { tt_.resize(mb); }

void Engine::new_game() {
    tt_.clear();
    for (auto& w : workers_) w->clear();
}

void Engine::set_position(const Position& root, const std::vector<Position>& history) {
    root_ = root;
    history_.clear();
    for (const Position& p : history) history_.push_back(p.key());
    // Count trailing reversible plies: consecutive positions whose men and piece counts are unchanged.
    int rev = 0;
    const Position* next = &root;
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        const bool reversible = it->men(BLACK) == next->men(BLACK) && it->men(WHITE) == next->men(WHITE) &&
                                it->piece_count() == next->piece_count();
        if (!reversible) break;
        ++rev;
        next = &*it;
    }
    root_.set_reversible_plies(rev);
}

SearchResult Engine::go(const Limits& limits, InfoFn on_info) {
    init_lmr(params);
    stop_ = false;
    tt_.new_search();

    TimeControl tc;
    tc.start = Clock::now();
    tc.node_limit = limits.nodes;
    if (limits.movetime_ms) {
        tc.soft_ms = tc.hard_ms = limits.movetime_ms;
    } else if (limits.time_ms) {
        const int mtg = limits.moves_to_go ? std::min(limits.moves_to_go, 40) : 32;
        const std::int64_t reserve = std::min<std::int64_t>(limits.time_ms / 10, 1000);
        const std::int64_t avail = std::max<std::int64_t>(1, limits.time_ms - reserve);
        const std::int64_t base = avail / mtg + limits.inc_ms * 3 / 4;
        tc.soft_ms = std::max<std::int64_t>(1, base * 6 / 10);
        tc.hard_ms = std::max<std::int64_t>(1, std::min<std::int64_t>(avail * 4 / 10, base * 3));
    }

    MoveList list;
    generate_moves(root_, list);
    SearchResult result;
    if (list.empty()) return result;
    std::vector<Move> root_moves(list.begin(), list.end());

    for (auto& w : workers_) w->setup(root_, history_, root_moves);

    std::vector<std::thread> helpers;
    for (std::size_t i = 1; i < workers_.size(); ++i)
        helpers.emplace_back([this, i, &limits, &tc] { workers_[i]->iterate(limits, tc, nullptr); });
    workers_[0]->iterate(limits, tc, &on_info);
    stop_ = true;
    for (auto& t : helpers) t.join();

    const Worker& main = *workers_[0];
    result.best = main.roots[0].move;
    result.score = main.roots[0].score == -SCORE_INF ? main.roots[0].prev_score : main.roots[0].score;
    result.depth = main.completed_depth;
    result.pv = main.roots[0].pv;
    for (const auto& w : workers_) result.nodes += w->nodes.load();
    result.time_ms = elapsed_ms(tc.start);
    return result;
}

}  // namespace regicide
