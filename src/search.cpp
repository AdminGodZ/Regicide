#include "search.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <optional>
#include <thread>

#include "egdb/egdb.hpp"
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

    Score evaluate(const Position& pos) const { return (*evaluator)(pos); }

    void clear() {
        std::memset(history, 0, sizeof(history));
        std::memset(cont, 0, sizeof(cont));
        std::memset(killers, 0, sizeof(killers));
    }

    void setup(const Position& root, const std::vector<Key>& game_keys, const std::vector<Move>& root_moves);
    void iterate(const Limits& limits, TimeControl& tc, const Engine::InfoFn* on_info);
    Score search(Position& pos, Score alpha, Score beta, int depth, int ply, bool cut_node, int qext);

    bool stopped() const { return eng.stop_.load(std::memory_order_relaxed); }
    void check_limits();
    bool is_repetition(const Position& pos, int ply) const;
    void update_quiet_stats(Color us, const Move& best, const Move* tried, int ntried, int depth, int ply);
    int quiet_score(Color us, const Move& m, int ply) const {
        return history[us][m.from][m.to] + (P.cont_hist ? cont[prev_to[ply]][m.from][m.to] : 0);
    }
    SearchInfo make_info(int depth, Score score) const;

    Engine& eng;
    TranspositionTable& tt;
    const int id;
    SearchParams P;
    TimeControl* tc = nullptr;
    const Evaluator* evaluator = nullptr;

    Position root;
    std::vector<RootMove> roots;
    std::vector<Key> keys;  // game history keys followed by the current search path
    std::size_t hist_len = 0;

    std::int16_t history[2][32][32];
    std::int16_t cont[33][32][32];  // [destination of the opponent's last move (32 = none)][from][to]
    Move killers[MAX_PLY + 8][2];
    int prev_to[MAX_PLY + 8];  // destination of the move that led to each ply (32 at the root)
    Move excluded[MAX_PLY + 8];  // move skipped by a singular-extension verification search at this ply
    bool has_excluded[MAX_PLY + 8] = {};
    Move pv[MAX_PLY + 8][MAX_PLY + 8];
    int pv_len[MAX_PLY + 8];

    std::atomic<std::uint64_t> nodes{0};
    std::atomic<std::uint64_t> tbhits{0};
    int egdb_pieces = 0;
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
    tbhits = 0;
    egdb_pieces = eng.egdb_pieces_;
    seldepth = 0;
    completed_depth = 0;
    P = eng.params;
    evaluator = eng.eval_.get();
}

bool Worker::is_repetition(const Position& pos, int ply) const {
    const long idx = long(hist_len) + ply;
    const long stop = std::max(0L, idx - pos.reversible_plies());
    for (long i = idx - 2; i >= stop; i -= 2)
        if (keys[i] == pos.key()) return true;
    return false;
}

void Worker::check_limits() {
    if (eng.stop_poll_ && eng.stop_poll_()) eng.stop();
    if (!tc) return;
    if (tc->hard_ms && elapsed_ms(tc->start) >= tc->hard_ms) eng.stop();
    if (tc->node_limit && nodes.load(std::memory_order_relaxed) * eng.threads_ >= tc->node_limit) eng.stop();
}

void Worker::update_quiet_stats(Color us, const Move& best, const Move* tried, int ntried, int depth, int ply) {
    const int bonus = std::min(depth * depth * 8, 4000);
    auto adjust = [&](const Move& m, int b) {
        std::int16_t& h = history[us][m.from][m.to];
        h = std::int16_t(h + b - h * std::abs(b) / 16384);
        std::int16_t& c = cont[prev_to[ply]][m.from][m.to];
        c = std::int16_t(c + b - c * std::abs(b) / 16384);
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

    // Each counter has a single writer (its worker), so a plain increment is enough; others only read it.
    const std::uint64_t n = nodes.load(std::memory_order_relaxed);
    nodes.store(n + 1, std::memory_order_relaxed);
    if ((n & 1023) == 0 && id == 0) check_limits();
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

        // Endgame database: exact result. Wins prefer the shortest path into the database.
        if (pos.piece_count() <= egdb_pieces) {
            const egdb::Wld r = egdb::probe(pos.pieces(BLACK), pos.pieces(WHITE), pos.kings(), pos.side_to_move());
            if (r != egdb::Wld::Unknown) {
                tbhits.store(tbhits.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
                return r == egdb::Wld::Draw ? SCORE_DRAW
                       : r == egdb::Wld::Win ? SCORE_DB_WIN - ply
                                             : -SCORE_DB_WIN + ply;
            }
        }
    }

    const bool excluding = has_excluded[ply];
    TTData tte;
    const bool tt_hit = !excluding && tt.probe(pos.key(), tte);
    const Score tt_score = tt_hit ? score_from_tt(tte.score, ply) : 0;
    if (!pv_node && tt_hit && tte.depth >= depth &&
        (tte.bound & (tt_score >= beta ? BOUND_LOWER : BOUND_UPPER)))
        return tt_score;

    // At the horizon only captures are searched, so quiet moves are generated only if the search goes on.
    MoveList moves;
    if (is_root)
        for (const RootMove& rm : roots) moves.push(rm.move);
    else if (depth > 0)
        generate_moves(pos, moves);
    else
        generate_captures(pos, moves);
    if (moves.empty() && (depth > 0 || !quiet_targets(pos, us))) return -SCORE_WIN + ply;

    // With a capture forced, nothing below uses the static eval (no pruning applies), so skip computing it.
    const bool forced_capture = !moves.empty() && moves[0].is_capture();
    const Score eval = tt_hit ? Score(tte.eval) : forced_capture ? 0 : evaluate(pos);

    if (depth <= 0 && !forced_capture) {
        // Horizon. If the opponent threatens a capture the static eval can't be trusted: look one ply further.
        if (qext > 0 && has_capture(pos, ~us)) {
            --qext;
            depth = 1;
            generate_moves(pos, moves);
        } else {
            return eval;
        }
    }

    // Razoring: far below alpha at low depth, check whether the horizon search agrees.
    if (!pv_node && !excluding && !forced_capture && depth > 0 && depth <= P.razor_depth &&
        eval + P.razor_margin * depth < alpha && !is_decisive(alpha)) {
        const Score s = search(pos, alpha - 1, alpha, 0, ply, cut_node, qext);
        if (stopped()) return 0;
        if (s < alpha) return s;
        pv_len[ply] = ply;
    }

    // Reverse futility pruning: far above beta at low depth, assume a quiet move holds it.
    if (!pv_node && !excluding && !forced_capture && depth > 0 && depth <= P.rfp_max_depth && !is_decisive(beta) &&
        eval - P.rfp_margin * depth >= beta && !has_capture(pos, ~us))
        return eval;

    // Internal iterative reduction: no hash move at high depth means this node is probably unimportant.
    if (depth >= 6 && !(tt_hit && tte.move.valid())) --depth;

    // Enhanced transposition cutoff: a child already known to refute this node from the hash table.
    if (!is_root && !excluding && depth >= P.etc_min_depth) {
        for (const Move& m : moves) {
            TTData ce;
            if (!tt.probe(pos.key_after(m), ce) || ce.depth < depth - 1 || !(ce.bound & BOUND_UPPER)) continue;
            const Score cs = -score_from_tt(ce.score, ply + 1);
            if (cs >= beta && !is_decisive(cs)) return cs;
        }
    }

    // ProbCut: if a shallower search clears beta by a margin, the full-depth search very likely clears beta.
    if (!pv_node && !excluding && !forced_capture && depth >= P.probcut_depth && !is_decisive(beta)) {
        const Score pb = beta + P.probcut_margin;
        const Score s = search(pos, pb - 1, pb, depth - P.probcut_reduction, ply, cut_node, qext);
        if (stopped()) return 0;
        if (s >= pb) return s;
        pv_len[ply] = ply;
    }

    const int ext = depth > 0 && moves.size() == 1 && P.single_reply_ext ? 1 : 0;

    // Singular extension: if every alternative to the hash move fails well below its score, the hash move is
    // forced in all but name and gets one extra ply. If an alternative also beats beta, cut (multi-cut).
    int tt_move_ext = 0;
    if (!is_root && !excluding && depth >= P.se_depth && tt_hit && tte.move.valid() && (tte.bound & BOUND_LOWER) &&
        tte.depth >= depth - 3 && !is_decisive(tt_score) && moves.size() > 1) {
        const int tt_i = find_move(moves, tte.move);
        if (tt_i >= 0) {
            const Score sb = tt_score - P.se_margin * depth / 8;
            excluded[ply] = moves[tt_i];
            has_excluded[ply] = true;
            const Score s = search(pos, sb - 1, sb, (depth - 1) / 2, ply, cut_node, qext);
            has_excluded[ply] = false;
            if (stopped()) return 0;
            pv_len[ply] = ply;
            if (s < sb) tt_move_ext = 1;
            else if (sb >= beta) return sb;
        }
    }

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
            s = quiet_score(us, m, ply);
        order[i] = s;
    }

    Score best = -SCORE_INF;
    int best_idx = -1;
    MoveList quiets;  // quiet moves searched without a cutoff

    for (std::size_t n = 0; n < moves.size(); ++n) {
        // Selection sort: bring the best-ordered remaining move to position n.
        std::size_t bi = n;
        for (std::size_t j = n + 1; j < moves.size(); ++j)
            if (order[j] > order[bi]) bi = j;
        std::swap(moves[n], moves[bi]);
        std::swap(order[n], order[bi]);
        const Move m = moves[n];
        if (excluding && m == excluded[ply]) continue;
        const bool quiet = !m.is_capture() && !is_promotion(pos, m);

        // Shallow pruning of late quiet moves.
        if (!pv_node && quiet && n > 0 && best > -SCORE_KNOWN_WIN) {
            if (depth <= P.lmp_depth && int(quiets.size()) >= P.lmp_base + depth * depth) continue;
            if (depth <= P.fut_depth && eval + P.fut_margin * depth <= alpha) continue;
        }

        Position child = pos;
        child.make_move(m);
        tt.prefetch(child.key());
        const int new_depth = depth - 1 + ext + (order[n] == (1 << 30) ? tt_move_ext : 0);
        prev_to[ply + 1] = m.to;

        Score s;
        if (n == 0) {
            s = -search(child, -beta, -alpha, new_depth, ply + 1, false, qext);
        } else {
            int r = 0;
            if (depth >= P.lmr_min_depth && quiet) {
                r = LMR[std::min(depth, 63)][std::min<int>(int(n), 63)];
                r += cut_node;
                r -= pv_node;
                r -= quiet_score(us, m, ply) / 6000;
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
                    if (quiet) update_quiet_stats(us, m, quiets.begin(), int(quiets.size()), depth, ply);
                    break;
                }
                alpha = s;
            }
        }
        if (quiet) quiets.push(m);
    }

    if (excluding) return best == -SCORE_INF ? alpha : best;  // verification search: don't store
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
    for (const auto& w : eng.workers_) {
        info.nodes += w->nodes.load(std::memory_order_relaxed);
        info.tbhits += w->tbhits.load(std::memory_order_relaxed);
    }
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

    prev_to[0] = 32;
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

bool SearchParams::set(const std::string& name, int value) {
    struct {
        const char* name;
        int SearchParams::* field;
    } static constexpr table[] = {
        {"aspiration_delta", &SearchParams::aspiration_delta}, {"rfp_margin", &SearchParams::rfp_margin},
        {"rfp_max_depth", &SearchParams::rfp_max_depth},       {"lmr_base", &SearchParams::lmr_base},
        {"lmr_mult", &SearchParams::lmr_mult},                 {"lmr_min_depth", &SearchParams::lmr_min_depth},
        {"threat_qext", &SearchParams::threat_qext},           {"single_reply_ext", &SearchParams::single_reply_ext},
        {"etc_min_depth", &SearchParams::etc_min_depth},       {"probcut_depth", &SearchParams::probcut_depth},
        {"probcut_margin", &SearchParams::probcut_margin},     {"probcut_reduction", &SearchParams::probcut_reduction},
        {"lmp_depth", &SearchParams::lmp_depth},               {"lmp_base", &SearchParams::lmp_base},
        {"fut_depth", &SearchParams::fut_depth},               {"fut_margin", &SearchParams::fut_margin},
        {"cont_hist", &SearchParams::cont_hist},               {"se_depth", &SearchParams::se_depth},
        {"se_margin", &SearchParams::se_margin},               {"razor_depth", &SearchParams::razor_depth},
        {"razor_margin", &SearchParams::razor_margin},
    };
    for (const auto& e : table)
        if (name == e.name) {
            this->*e.field = value;
            return true;
        }
    return false;
}

bool SearchParams::set_all(const std::string& spec) {
    bool ok = true;
    std::size_t pos = 0;
    while (pos < spec.size()) {
        std::size_t end = spec.find(',', pos);
        if (end == std::string::npos) end = spec.size();
        const std::string item = spec.substr(pos, end - pos);
        const std::size_t eq = item.find('=');
        if (eq == std::string::npos || !set(item.substr(0, eq), std::stoi(item.substr(eq + 1)))) ok = false;
        pos = end + 1;
    }
    return ok;
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

// Perfect play from the database when the root is in it and decided: the winner follows moves-to-conversion
// downhill (so the win is always realised), the loser maximises it. Draws are left to the search.
std::optional<std::pair<Move, Score>> Engine::egdb_root_move(const Position& root, const MoveList& list) const {
    if (!egdb_pieces_ || root.piece_count() > std::min(egdb_pieces_, egdb::max_mtc_pieces())) return std::nullopt;
    auto probe = [](const Position& p) {
        return egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
    };
    auto mtc = [](const Position& p) {
        return egdb::probe_mtc(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
    };
    const egdb::Wld r = probe(root);
    if (r != egdb::Wld::Win && r != egdb::Wld::Loss) return std::nullopt;
    const int root_mtc = mtc(root);
    if (root_mtc < 0) return std::nullopt;

    const bool winning = r == egdb::Wld::Win;
    std::optional<Move> best;
    int best_key = 0;
    for (const Move& m : list) {
        Position c = root;
        c.make_move(m);
        MoveList replies;
        generate_moves(c, replies);
        const bool converting = m.is_capture() || !(root.kings() & bit(m.from));
        if (winning) {
            // The child must be lost for the opponent: either no moves at all or a database loss.
            if (!replies.empty() && probe(c) != egdb::Wld::Loss) continue;
            // At MTC 0 play a converting move; otherwise step to a child one ply closer to conversion.
            const int cm = replies.empty() ? 0 : mtc(c);
            const int key = root_mtc == 0 ? (converting ? 0 : 1000 + cm) : (converting ? -1 : cm);
            if (!best || key < best_key) best = m, best_key = key;
        } else {
            const int cm = replies.empty() ? -1000 : mtc(c);
            if (!best || cm > best_key) best = m, best_key = cm;
        }
    }
    if (!best) return std::nullopt;
    return std::pair{*best, winning ? SCORE_DB_WIN - root_mtc : -SCORE_DB_WIN + root_mtc};
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
    } else if (limits.target_ms) {
        tc.soft_ms = std::max(1, limits.target_ms * 6 / 10);
        tc.hard_ms = std::max(1, limits.target_ms * 5 / 2);
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

    if (const auto db = egdb_root_move(root_, list); db.has_value() && !limits.infinite) {
        result.best = db->first;
        result.score = db->second;
        result.pv = {db->first};
        result.time_ms = elapsed_ms(tc.start);
        if (on_info) {
            SearchInfo info;
            info.score = result.score;
            info.pv = result.pv;
            on_info(info);
        }
        return result;
    }

    if (book_ && limits.use_book && !limits.infinite && !book_->empty()) {
        book_seed_ = book_seed_ * 6364136223846793005ull + 1442695040888963407ull;
        if (auto m = book_->probe(root_, book_margin_, book_seed_ >> 33)) {
            for (const Book::Entry& e : book_->lookup(root_))
                if (Book::to_move(root_, e) == m) result.score = e.score;
            result.best = *m;
            result.pv = {*m};
            if (on_info) {
                SearchInfo info;
                info.score = result.score;
                info.pv = result.pv;
                on_info(info);
            }
            return result;
        }
    }
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
