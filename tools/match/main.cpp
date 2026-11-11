// match: play engine A against engine B over the cbhost protocol.
//
// Every opening is played twice with colours reversed. Games run concurrently, each with its
// own pair of engine processes. The referee (tools/common) validates every move and
// adjudicates: no legal moves = loss, threefold repetition = draw, no-progress rule = draw.
// Results go to per-game PDN files, a combined PDN, a CSV, and a summary with W/D/L, draw rate,
// pentanomial Elo with 95% CI, LOS and an optional GSPRT that can stop the match early.

#include <signal.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "cb_api.h"
#include "checkers.hpp"
#include "engine_process.hpp"
#include "openings.hpp"
#include "stats.hpp"

namespace fs = std::filesystem;
using namespace match;
using cbt::Board;
using cbt::Color;

namespace {

std::atomic<bool> g_interrupted{false};
void on_sigint(int) { g_interrupted = true; }

// ---------------------------------------------------------------------------------------------
// Configuration

enum class TimeMode { CbMatch, Plain, Exact, Incremental };

struct Config {
    EngineSpec spec[2];
    std::string deck = "acf156";
    int rounds = 1;
    long max_pairs = 0;  // 0 = whole deck x rounds
    int concurrency = 6;

    TimeMode time_mode = TimeMode::CbMatch;
    double movetime = 0.1;
    double tc_initial = 0, tc_increment = 0;
    bool time_forfeit = true;  // incremental time control only
    double stop_factor = 4.0;  // fixed time: send `stop` after stop_factor * maxtime + grace
    double grace = 0.25;
    double kill_after = 2.0;   // then kill (loss) this long after `stop`

    // Moves per side without a capture or man move (draw after 2N - 1 plies); 0 = off. The default is the rule the
    // engine's search assumes (NO_PROGRESS_PLIES = 79 in src/search.hpp), implemented here independently;
    // tools/selfplay.cpp (80 plies) and tools/datagen.cpp (60) use limits of their own.
    int no_progress = 40;
    int max_plies = 1000;
    bool accept_draw_claims = false;
    // CheckerBoard's engine match with its default "early adjudication": a draw, win or loss claim ends the game, and
    // a game still running after cb_move_limit engine moves (0 = off) has no result.
    bool cb_claims = false;
    int cb_move_limit = 0;

    bool sprt = false;
    SprtConfig sprt_cfg;

    std::string out_dir;
    std::string cbhost;
    bool pdn_comments = false;
    bool fresh_engines = false;  // new engine processes for every game
    bool pairs_together = false;  // a worker plays both games of an opening pair, one after the other
    bool quiet = false;
};

std::string tc_description(const Config& c) {
    char buf[128];
    switch (c.time_mode) {
        case TimeMode::CbMatch: std::snprintf(buf, sizeof buf, "%gs/move (CheckerBoard match rules)", c.movetime); break;
        case TimeMode::Plain: std::snprintf(buf, sizeof buf, "%gs/move", c.movetime); break;
        case TimeMode::Exact: std::snprintf(buf, sizeof buf, "%gs/move exact", c.movetime); break;
        case TimeMode::Incremental: std::snprintf(buf, sizeof buf, "%gs+%gs", c.tc_initial, c.tc_increment); break;
    }
    return buf;
}

// Endgame-database adjudication hook (not wired to a database yet). Given a position, return the
// result for the side to move (+1 win, 0 draw, -1 loss) when it is known exactly, e.g. from
// Regicide's own EGDB once it exists. The referee then ends the game with that result.
struct Adjudicator {
    virtual ~Adjudicator() = default;
    virtual std::optional<int> probe(const Board& b) = 0;
};
Adjudicator* g_adjudicator = nullptr;

// ---------------------------------------------------------------------------------------------
// Games

// Unfinished: stopped by --cb-move-limit; recorded with result * but not scored. Aborted: interrupted, not recorded.
enum class Winner { Black, White, Draw, Unfinished, Aborted };

struct GameRecord {
    long index = 0;  // 0-based game number
    long pair = 0;
    int leg = 0;     // 0: A has black, 1: A has white
    const Opening* opening = nullptr;
    std::string black, white;
    Winner winner = Winner::Aborted;
    std::string termination;   // human readable
    std::string category;      // for counting
    std::vector<std::string> moves;  // engine moves (after the opening), PDN
    std::vector<std::string> comments;
    int worker = 0;        // which worker played it
    int engine_games[2] = {0, 0};  // by engine (A, B): games its current process had played before this one
    std::string first_claim = "none";  // "ply P C CLAIM": the game's first win, loss or draw claim of an engine
    int claims[2][3] = {};             // by engine (A, B): its win, loss and draw claims
    double time_used[2] = {0, 0};  // by colour
    int moves_made[2] = {0, 0};
    double max_time[2] = {0, 0};
    bool a_is_black() const { return leg == 0; }
    double a_score() const {
        if (winner == Winner::Draw) return 0.5;
        bool black_won = winner == Winner::Black;
        return black_won == a_is_black() ? 1.0 : 0.0;
    }
};

const char* pdn_result(Winner w) {
    switch (w) {
        case Winner::Black: return "1-0";  // English checkers PDN: first player (Black) first
        case Winner::White: return "0-1";
        case Winner::Draw: return "1/2-1/2";
        default: return "*";
    }
}

struct SideClock {
    double remaining = 0;
    bool first = true;
};

// Play one game. eng[0] plays black, eng[1] white. Returns with g filled in; engines that
// crashed or were killed are left not alive() for the caller to restart.
void play_game(const Config& cfg, Engine* eng[2], GameRecord& g) {
    const Opening& op = *g.opening;
    Board b = op.start;
    std::vector<std::string> history;  // all moves from the start position given to engines
    for (const auto& m : op.played) history.push_back(cbt::move_to_pdn(m));
    const std::string position_base = op.fen.empty() ? "position startpos" : "position fen " + op.fen;

    std::vector<Board> window{b};  // positions since the last conversion move, for repetition
    int nonconv = 0;
    SideClock clk[2];
    if (cfg.time_mode == TimeMode::Incremental) clk[0].remaining = clk[1].remaining = cfg.tc_initial + cfg.tc_increment;

    for (int e = 0; e < 2; ++e) {
        if (!eng[e]->new_game()) {
            g.winner = e == 0 ? Winner::White : Winner::Black;
            g.termination = eng[e]->name() + " did not start a new game";
            g.category = "crash";
            eng[e]->kill();
            return;
        }
    }

    auto lose = [&](Color side, std::string why, std::string category) {
        g.winner = side == Color::Black ? Winner::White : Winner::Black;
        g.termination = std::move(why);
        g.category = std::move(category);
    };
    // CheckerBoard's move limit counts search attempts (AutoThreadFunc), and finding that the side to move has no move
    // is an attempt of its own (SearchThreadFunc). Once the count passes the limit the game is recorded as unknown,
    // whatever that attempt brought: a claim, a draw by the rules or the missing move (update_match_stats).
    // past_limit(attempts) then ends the game as unfinished and returns true.
    auto past_limit = [&](int attempts) {
        if (cfg.cb_move_limit <= 0 || attempts <= cfg.cb_move_limit) return false;
        g.winner = Winner::Unfinished;
        g.termination = "move limit, no result";
        g.category = "move limit";
        return true;
    };

    for (int ply = 0;; ++ply) {
        if (g_interrupted) {
            g.winner = Winner::Aborted;
            g.termination = "interrupted";
            return;
        }
        auto legal = b.legal_moves();
        if (legal.empty()) {
            if (past_limit(ply + 1)) return;
            lose(b.stm, std::string(cbt::color_name(b.stm)) + " has no legal moves", "no legal moves");
            return;
        }
        if (ply >= cfg.max_plies) {
            g.winner = Winner::Draw;
            g.termination = "adjudicated draw after " + std::to_string(ply) + " plies (max plies)";
            g.category = "max plies";
            return;
        }

        const int side = b.stm == Color::Black ? 0 : 1;
        Engine& e = *eng[side];
        SideClock& c = clk[side];

        double maxtime = cfg.movetime;
        std::string go = "go";
        char buf[160];
        Clock::time_point now = Clock::now(), stop_at, kill_at;
        auto after = [&](double s) { return now + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(s)); };
        switch (cfg.time_mode) {
            case TimeMode::CbMatch:
                c.remaining += cfg.movetime;
                maxtime = cbt::cb_maxtime_for_non_incremental_tc(c.remaining, cfg.movetime);
                break;
            case TimeMode::Plain:
            case TimeMode::Exact:
                break;
            case TimeMode::Incremental:
                maxtime = cbt::cb_maxtime_for_incremental_tc(c.remaining, cfg.tc_initial, cfg.tc_increment);
                break;
        }
        std::snprintf(buf, sizeof buf, " maxtime %.4f", maxtime);
        go += buf;
        if (c.first) go += " reset";  // CheckerBoard: CB_RESET_MOVES on each side's first search
        if (cfg.time_mode == TimeMode::Exact) go += " exact";
        if (cfg.time_mode == TimeMode::Incremental) {
            std::snprintf(buf, sizeof buf, " inc %.4f remaining %.4f", cfg.tc_increment, std::max(c.remaining, 0.0));
            go += buf;
            stop_at = after(std::max(c.remaining, 0.0) + cfg.grace);
        } else {
            stop_at = after(cfg.stop_factor * maxtime + cfg.grace);
        }
        kill_at = stop_at + std::chrono::duration_cast<Clock::duration>(
                                std::chrono::duration<double>(std::max(cfg.kill_after, maxtime)));
        c.first = false;

        std::string position = position_base;
        if (!history.empty()) {
            position += " moves";
            for (const auto& m : history) position += " " + m;
        }
        GoResult r = e.go(position, go, stop_at, kill_at);
        if (r.status != GoResult::Status::Ok) {
            // An interrupt of the whole process group ends the engines too. Their exit is not a result: give the
            // runner's own signal a moment to arrive, and leave the game undecided.
            if (!g_interrupted) std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (g_interrupted) {
                g.winner = Winner::Aborted;
                g.termination = "interrupted";
                return;
            }
        }
        g.time_used[side] += r.elapsed;
        g.max_time[side] = std::max(g.max_time[side], r.elapsed);
        g.moves_made[side]++;

        switch (r.status) {
            case GoResult::Status::Ok: break;
            case GoResult::Status::Crash: lose(b.stm, e.name() + " crashed: " + r.detail, "crash"); e.kill(); return;
            case GoResult::Status::Timeout: lose(b.stm, e.name() + " timed out: " + r.detail, "timeout"); return;
            case GoResult::Status::Illegal: lose(b.stm, e.name() + " illegal move: " + r.detail, "illegal move"); return;
            case GoResult::Status::Error:
                lose(b.stm, e.name() + " protocol error: " + r.detail, "protocol error");
                e.kill();
                return;
        }
        std::string err;
        auto m = cbt::parse_move(b, r.move, &err);
        if (!m) {
            lose(b.stm, e.name() + " illegal move: " + err, "illegal move");
            return;
        }

        if (cfg.time_mode == TimeMode::CbMatch) {
            c.remaining -= r.charged();
        } else if (cfg.time_mode == TimeMode::Incremental) {
            // As in CheckerBoard, the clock runs while the engine thinks, not while the host prepares the call.
            const double used = r.charged();
            double before = c.remaining;
            c.remaining += cfg.tc_increment - used;
            if (cfg.time_forfeit && used > before + cfg.grace) {
                std::snprintf(buf, sizeof buf, " lost on time (%.3fs used, %.3fs left)", used, before);
                lose(b.stm, e.name() + buf, "time forfeit");
                return;
            }
        }

        const std::string pdn = cbt::move_to_pdn(*m);
        history.push_back(pdn);
        g.moves.push_back(pdn);
        // The engine's claim (getmove's return value) with the move, whatever the runner does with it.
        const int kind = r.claim == "win" ? 0 : r.claim == "loss" ? 1 : r.claim == "draw" ? 2 : -1;
        if (kind >= 0) {
            ++g.claims[(side == 0) == g.a_is_black() ? 0 : 1][kind];
            if (g.first_claim == "none")
                g.first_claim = "ply " + std::to_string(history.size()) + (side == 0 ? " B " : " W ") + r.claim;
        }
        if (cfg.pdn_comments) {
            std::snprintf(buf, sizeof buf, "%.2fs", r.elapsed);
            std::string info = r.info;  // free text of the engine: a brace in it would end the PDN comment
            std::ranges::replace(info, '{', '(');
            std::ranges::replace(info, '}', ')');
            g.comments.push_back(std::string(buf) + (info.empty() ? "" : " " + info) +
                                 (kind >= 0 ? " claim=" + r.claim : ""));
        }
        if (past_limit(ply + 1)) return;  // the move is recorded, its claim and the board are not looked at
        const bool conversion = b.is_conversion(*m);
        b = b.after(*m);
        // --cb-claims: a win claim on a move that leaves the opponent without a move ends the game within this
        // attempt, as in CheckerBoard, with the board's result. Without that claim the missing move is found in an
        // attempt of its own (past_limit, next turn of the loop); a draw or loss claim keeps the board's result.
        if (cfg.cb_claims && kind == 0 && b.legal_moves().empty()) {
            lose(b.stm, std::string(cbt::color_name(b.stm)) + " has no legal moves", "no legal moves");
            return;
        }

        if (conversion) {
            nonconv = 0;
            window.assign(1, b);
        } else {
            ++nonconv;
            window.push_back(b);
            // A move that leaves the opponent without a move wins (next turn of the loop), whatever the counters say.
            if (b.legal_moves().empty()) continue;
            if (std::count(window.begin(), window.end(), b) >= 3) {
                g.winner = Winner::Draw;
                g.termination = "threefold repetition";
                g.category = "repetition";
                return;
            }
            // CheckerBoard: 40 moves by one side and 39 by the other without a conversion.
            if (cfg.no_progress > 0 && nonconv >= 2 * cfg.no_progress - 1) {
                g.winner = Winner::Draw;
                g.termination = std::to_string(cfg.no_progress) + "-move rule (no capture or man move)";
                g.category = "no progress";
                return;
            }
        }
        // --cb-claims: the claim ends the game after its move, unless the move left the opponent without a move (the
        // board's result, next turn of the loop).
        if (cfg.cb_claims && kind >= 0 && !b.legal_moves().empty()) {
            const Color mover = side == 0 ? Color::Black : Color::White;
            if (kind == 0) lose(cbt::opposite(mover), "win claimed by " + e.name(), "win claim");
            else if (kind == 1) lose(mover, "loss conceded by " + e.name(), "loss claim");
            else {
                g.winner = Winner::Draw;
                g.termination = "draw claimed by " + e.name();
                g.category = "draw claim";
            }
            return;
        }
        if (cfg.accept_draw_claims && r.claim == "draw") {
            g.winner = Winner::Draw;
            g.termination = "draw claimed by " + e.name();
            g.category = "draw claim";
            return;
        }
        if (g_adjudicator) {
            if (auto v = g_adjudicator->probe(b)) {
                if (*v == 0) {
                    g.winner = Winner::Draw;
                } else {
                    bool stm_wins = *v > 0;
                    g.winner = (stm_wins == (b.stm == Color::Black)) ? Winner::Black : Winner::White;
                }
                g.termination = "endgame database adjudication";
                g.category = "egdb";
                return;
            }
        }
    }
}

std::string pdn_text(const Config& cfg, const GameRecord& g) {
    std::ostringstream o;
    std::time_t t = std::time(nullptr);
    char date[32];
    std::strftime(date, sizeof date, "%Y.%m.%d", std::localtime(&t));
    const Opening& op = *g.opening;
    o << "[Event \"" << g.black << " vs " << g.white << "\"]\n";
    o << "[Site \"Regicide match runner\"]\n";
    o << "[Date \"" << date << "\"]\n";
    o << "[Round \"" << g.pair + 1 << "." << g.leg + 1 << "\"]\n";
    o << "[Black \"" << g.black << "\"]\n";
    o << "[White \"" << g.white << "\"]\n";
    o << "[Result \"" << pdn_result(g.winner) << "\"]\n";
    o << "[GameType \"21\"]\n";
    if (!op.fen.empty()) o << "[SetUp \"1\"]\n[FEN \"" << op.fen << "\"]\n";
    o << "[Opening \"" << op.name << (op.moves.empty() ? "" : ": " + op.moves) << "\"]\n";
    o << "[TimeControl \"" << tc_description(cfg) << "\"]\n";
    o << "[Termination \"" << g.termination << "\"]\n";
    // An engine may carry its hash table or other state from one game to the next: which worker played the game
    // and how many games each engine's process had played before.
    o << "[Worker \"" << g.worker << "\"]\n[EngineGamesA \"" << g.engine_games[0] << "\"]\n[EngineGamesB \""
      << g.engine_games[1] << "\"]\n";
    // The engines' claims (getmove's return values): the first one, and how many of each kind each engine made.
    o << "[FirstClaim \"" << g.first_claim << "\"]\n";
    for (int e = 0; e < 2; ++e)
        o << "[Claims" << (e == 0 ? "A" : "B") << " \"w" << g.claims[e][0] << " l" << g.claims[e][1] << " d"
          << g.claims[e][2] << "\"]\n";

    std::vector<std::string> all;
    for (const auto& m : op.played) all.push_back(cbt::move_to_pdn(m));
    const std::size_t book = all.size();
    all.insert(all.end(), g.moves.begin(), g.moves.end());

    Color stm = Color::Black;
    if (!op.fen.empty())
        if (auto f = Board::from_fen(op.fen)) stm = f->stm;
    std::string body;
    int number = 1;
    for (std::size_t i = 0; i < all.size(); ++i) {
        std::string tok;
        if (stm == Color::Black) tok = std::to_string(number) + ". ";
        else if (i == 0) tok = std::to_string(number) + "... ";
        tok += all[i];
        if (i >= book && i - book < g.comments.size()) tok += " {" + g.comments[i - book] + "}";
        if (stm == Color::White) ++number;
        stm = cbt::opposite(stm);
        body += (body.empty() ? "" : " ") + tok;
    }
    body += (body.empty() ? "" : " ") + std::string(pdn_result(g.winner));
    // Wrap at 80 columns.
    std::string line;
    std::istringstream words(body);
    std::string w;
    while (words >> w) {
        if (!line.empty() && line.size() + 1 + w.size() > 80) {
            o << line << "\n";
            line.clear();
        }
        line += (line.empty() ? "" : " ") + w;
    }
    o << line << "\n\n";
    return o.str();
}

// ---------------------------------------------------------------------------------------------
// Match bookkeeping

struct Match {
    const Config& cfg;
    std::vector<Opening> openings;
    long total_games = 0;
    std::string names[2];

    std::mutex mu;
    long next_task = 0;
    bool stop_scheduling = false;
    std::string stop_reason;
    std::string fatal;
    Stats stats;
    long done = 0;
    long a_black[3] = {0, 0, 0}, a_white[3] = {0, 0, 0};  // W, D, L for A by colour
    long black_wins = 0, white_wins = 0, draws_total = 0;
    std::map<std::string, long> terminations;
    long unfinished = 0;  // games stopped by --cb-move-limit
    std::map<long, std::pair<int, double>> pair_partial;  // pair -> (games done, A score)
    double engine_time[2] = {0, 0};
    long engine_moves[2] = {0, 0};
    double engine_max[2] = {0, 0};
    std::vector<GameRecord> records;
    std::ofstream all_pdn, csv;

    explicit Match(const Config& c) : cfg(c) {}

    const Opening& opening_for(long game) const { return openings[(game / 2) % openings.size()]; }

    // Openings with a fixed colour for A (file field A=B / A=W) play only that leg (0: A black, 1: A white).
    bool skipped(long task) const {
        const char c = opening_for(task).a_colour;
        return c && (task % 2) != (c == 'B' ? 0 : 1);
    }

    std::optional<long> take_task() {
        std::lock_guard lock(mu);
        while (next_task < total_games && skipped(next_task)) ++next_task;
        if (stop_scheduling || g_interrupted || next_task >= total_games) return std::nullopt;
        const long task = next_task++;
        // --pairs-together: the worker that takes a pair's first game also plays its second (take_second_leg).
        if (cfg.pairs_together && task % 2 == 0) ++next_task;
        return task;
    }

    // --pairs-together: the second game of the pair whose first game was `task`, unless the opening has only one game
    // or the match stops (then the pair stays incomplete, as a pair cut off by a stop always is).
    std::optional<long> take_second_leg(long task) {
        std::lock_guard lock(mu);
        if (task % 2 || task + 1 >= total_games || skipped(task + 1) || stop_scheduling || g_interrupted)
            return std::nullopt;
        return task + 1;
    }

    std::string line_stats() {
        char buf[512];
        Estimate p = stats.pentanomial();
        std::string sprt;
        if (cfg.sprt) {
            char s[96];
            std::snprintf(s, sizeof s, " LLR %.2f [%.2f, %.2f]", stats.llr(cfg.sprt_cfg), cfg.sprt_cfg.lower(), cfg.sprt_cfg.upper());
            sprt = s;
        }
        std::snprintf(buf, sizeof buf, "A +%ld =%ld -%ld (%.1f%%, draws %.0f%%) | pairs %ld Elo %s LOS %.1f%%%s",
                      stats.wins, stats.draws, stats.losses, 100.0 * (stats.wins + 0.5 * stats.draws) / std::max(1L, stats.games()),
                      100 * stats.draw_rate(), stats.pairs(), stats.pairs() ? format_elo(p).c_str() : "n/a", 100 * p.los,
                      sprt.c_str());
        return buf;
    }

    void record(GameRecord g) {
        std::lock_guard lock(mu);
        if (g.winner == Winner::Aborted) return;
        // An unfinished game has no score: it is written out but left out of the results, and its pair stays
        // incomplete (not in the pentanomial count), as a pair cut off by a stop does.
        const bool scored = g.winner != Winner::Unfinished;
        const double s = g.a_score();
        if (scored) {
            if (s == 1) ++stats.wins;
            else if (s == 0) ++stats.losses;
            else ++stats.draws;
            long* bycol = g.a_is_black() ? a_black : a_white;
            ++bycol[s == 1 ? 0 : s == 0 ? 2 : 1];
            if (g.winner == Winner::Black) ++black_wins;
            else if (g.winner == Winner::White) ++white_wins;
            else ++draws_total;
            ++terminations[g.category];
            auto& pp = pair_partial[g.pair];
            pp.first++;
            pp.second += s;
            if (pp.first == 2) {
                ++stats.penta[int(std::lround(pp.second * 2))];
                pair_partial.erase(g.pair);
            }
        } else {
            ++unfinished;
        }
        for (int c = 0; c < 2; ++c) {
            int engine = (c == 0) == g.a_is_black() ? 0 : 1;
            engine_time[engine] += g.time_used[c];
            engine_moves[engine] += g.moves_made[c];
            engine_max[engine] = std::max(engine_max[engine], g.max_time[c]);
        }
        ++done;

        char name[32];
        std::snprintf(name, sizeof name, "%05ld.pdn", g.index + 1);
        std::string text = pdn_text(cfg, g);
        std::ofstream(fs::path(cfg.out_dir) / "games" / name) << text;
        all_pdn << text << std::flush;
        csv << g.index + 1 << ',' << g.pair + 1 << ",\"" << g.opening->name << "\",\"" << g.black << "\",\"" << g.white
            << "\"," << pdn_result(g.winner) << ',';
        if (scored) csv << s;  // an unfinished game has no score
        csv << ",\"" << g.category << "\"," << g.moves.size() << ",\"" << g.first_claim << "\"\n" << std::flush;

        if (!cfg.quiet) {
            std::printf("[%4ld/%ld] %-22.22s %s-%s %-7s %-18.18s | %s\n", done, total_games, g.opening->name.c_str(),
                        g.a_is_black() ? "A" : "B", g.a_is_black() ? "B" : "A", pdn_result(g.winner), g.category.c_str(),
                        line_stats().c_str());
            std::fflush(stdout);
        }
        if (g.category == "crash" || g.category == "timeout" || g.category == "illegal move" ||
            g.category == "protocol error" || g.category == "time forfeit") {
            std::printf("    game %ld: %s\n", g.index + 1, g.termination.c_str());
        }
        if (cfg.sprt && !stop_scheduling && stats.pairs() >= cfg.sprt_cfg.min_pairs) {
            double llr = stats.llr(cfg.sprt_cfg);
            if (llr >= cfg.sprt_cfg.upper() || llr <= cfg.sprt_cfg.lower()) {
                stop_scheduling = true;
                char buf[160];
                std::snprintf(buf, sizeof buf, "SPRT %s: LLR %.2f crossed %.2f (elo0 %g, elo1 %g)",
                              llr > 0 ? "accepted H1" : "accepted H0", llr,
                              llr > 0 ? cfg.sprt_cfg.upper() : cfg.sprt_cfg.lower(), cfg.sprt_cfg.elo0, cfg.sprt_cfg.elo1);
                stop_reason = buf;
                std::printf("    %s; finishing games in progress\n", buf);
            }
        }
        records.push_back(std::move(g));
    }

    void set_fatal(const std::string& msg) {
        std::lock_guard lock(mu);
        if (fatal.empty()) fatal = msg;
        stop_scheduling = true;
    }

    std::string summary() {
        std::lock_guard lock(mu);
        std::ostringstream o;
        char buf[256];
        o << "Match: A = " << names[0] << ", B = " << names[1] << "\n";
        o << "Openings: " << cfg.deck << " (" << openings.size() << " openings x " << cfg.rounds
          << " rounds, each played with both colours)\n";
        o << "Time control: " << tc_description(cfg) << "; no-progress rule: "
          << (cfg.no_progress ? std::to_string(cfg.no_progress) + " moves" : std::string("off")) << "\n";
        o << "Games: " << stats.games() << " of " << total_games << " scheduled";
        if (!stop_reason.empty()) o << " (stopped: " << stop_reason << ")";
        if (g_interrupted) o << " (interrupted)";
        o << "\n\n";
        std::snprintf(buf, sizeof buf, "A: +%ld =%ld -%ld   score %.1f/%ld = %.2f%%   draw rate %.1f%%\n", stats.wins,
                      stats.draws, stats.losses, stats.wins + 0.5 * stats.draws, stats.games(),
                      100.0 * (stats.wins + 0.5 * stats.draws) / std::max(1L, stats.games()), 100 * stats.draw_rate());
        o << buf;
        std::snprintf(buf, sizeof buf, "A as black: +%ld =%ld -%ld   A as white: +%ld =%ld -%ld\n", a_black[0], a_black[1],
                      a_black[2], a_white[0], a_white[1], a_white[2]);
        o << buf;
        std::snprintf(buf, sizeof buf, "By colour: black won %ld, white won %ld, drawn %ld\n", black_wins, white_wins,
                      draws_total);
        o << buf;
        std::snprintf(buf, sizeof buf, "Pairs: %ld   pentanomial [LL LD DD/WL WD WW] = [%ld %ld %ld %ld %ld]\n",
                      stats.pairs(), stats.penta[0], stats.penta[1], stats.penta[2], stats.penta[3], stats.penta[4]);
        o << buf;
        Estimate p = stats.pentanomial(), t = stats.trinomial();
        if (p.n) {
            std::snprintf(buf, sizeof buf, "Elo (pentanomial, pairs): %s 95%% CI, score %.4f +- %.4f, LOS %.2f%%\n",
                          format_elo(p).c_str(), p.score, 1.96 * p.se, 100 * p.los);
            o << buf;
        }
        if (t.n) {
            std::snprintf(buf, sizeof buf, "Elo (trinomial, games):   %s 95%% CI, LOS %.2f%%\n", format_elo(t).c_str(),
                          100 * t.los);
            o << buf;
        }
        if (cfg.sprt) {
            std::snprintf(buf, sizeof buf, "SPRT elo0=%g elo1=%g alpha=%g beta=%g: LLR %.3f, bounds [%.3f, %.3f] -> %s\n",
                          cfg.sprt_cfg.elo0, cfg.sprt_cfg.elo1, cfg.sprt_cfg.alpha, cfg.sprt_cfg.beta,
                          stats.llr(cfg.sprt_cfg), cfg.sprt_cfg.lower(), cfg.sprt_cfg.upper(),
                          stop_reason.empty() ? "inconclusive" : stop_reason.c_str());
            o << buf;
        }
        o << "Terminations:";
        for (const auto& [k, v] : terminations) o << " " << k << " " << v << ";";
        o << "\n";
        if (cfg.cb_move_limit > 0) o << "Unfinished (move limit): " << unfinished << "\n";
        for (int e = 0; e < 2; ++e) {
            std::snprintf(buf, sizeof buf, "%s %s: %ld moves, avg %.3fs/move, max %.3fs\n", e == 0 ? "A" : "B",
                          names[e].c_str(), engine_moves[e], engine_time[e] / std::max(1L, engine_moves[e]), engine_max[e]);
            o << buf;
        }
        if (!fatal.empty()) o << "FATAL: " << fatal << "\n";
        return o.str();
    }
};

void worker(Match& m, int id) {
    const Config& cfg = m.cfg;
    std::unique_ptr<Engine> eng[2];
    for (int e = 0; e < 2; ++e) {
        std::string log = (fs::path(cfg.out_dir) / "logs" / (cfg.spec[e].label + "-" + std::to_string(id) + ".log")).string();
        eng[e] = std::make_unique<Engine>(cfg.spec[e], log);
    }
    bool launched = false;
    int engine_games[2] = {0, 0};  // by engine: games its current process has played
    for (auto task = m.take_task(); task;) {
        std::string err;
        if (cfg.fresh_engines && launched)
            for (auto& e : eng) e->kill();
        for (int e = 0; e < 2; ++e) {
            if (eng[e]->alive()) continue;
            engine_games[e] = 0;  // a new process (the first, a fresh one, or one that replaces a dead engine)
            if (!(launched ? eng[e]->restart(err) : eng[e]->launch(err))) {
                m.set_fatal("worker " + std::to_string(id) + ": " + err);
                return;
            }
        }
        launched = true;
        {
            std::lock_guard lock(m.mu);
            if (m.names[0].empty()) m.names[0] = eng[0]->name();
            if (m.names[1].empty()) m.names[1] = eng[1]->name();
        }
        GameRecord g;
        g.index = *task;
        g.pair = *task / 2;
        g.leg = int(*task % 2);
        g.opening = &m.opening_for(*task);
        Engine* by_colour[2] = {eng[g.leg == 0 ? 0 : 1].get(), eng[g.leg == 0 ? 1 : 0].get()};
        g.black = by_colour[0]->name();
        g.white = by_colour[1]->name();
        g.worker = id;
        for (int e = 0; e < 2; ++e) g.engine_games[e] = engine_games[e]++;
        play_game(cfg, by_colour, g);
        m.record(std::move(g));
        task = cfg.pairs_together ? m.take_second_leg(*task) : std::nullopt;
        if (!task) task = m.take_task();
    }
    for (auto& e : eng) e->kill();
}

// ---------------------------------------------------------------------------------------------
// Command line

std::string self_dir(const char* argv0) {
#ifdef __APPLE__
    char buf[4096];
    uint32_t size = sizeof buf;
    if (_NSGetExecutablePath(buf, &size) == 0) return fs::canonical(buf).parent_path().string();
#endif
    std::error_code ec;
    auto p = fs::canonical("/proc/self/exe", ec);
    if (!ec) return p.parent_path().string();
    return fs::absolute(argv0).parent_path().string();
}

std::vector<std::string> split_command(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    bool has = false;
    char quote = 0;
    for (char ch : s) {
        if (quote) {
            if (ch == quote) quote = 0;
            else cur += ch;
        } else if (ch == '"' || ch == '\'') {
            quote = ch;
            has = true;
        } else if (ch == ' ' || ch == '\t') {
            if (has || !cur.empty()) out.push_back(cur);
            cur.clear();
            has = false;
        } else {
            cur += ch;
        }
    }
    if (has || !cur.empty()) out.push_back(cur);
    return out;
}

void usage() {
    std::printf(R"(usage: match -a ENGINE -b ENGINE [options]

Engines (A is the engine under test; results are reported from A's point of view):
  -a, -b SPEC              a CheckerBoard engine .dylib/.so (run through cbhost), or
                           exec:COMMAND for a program that speaks the cbhost protocol itself
  --name-a, --name-b NAME  display name (default: the engine's own name)
  --option-a, --option-b TEXT
                           enginecommand to send after startup, e.g. "set hashsize 64"
                           (repeatable). The match does not start if an engine rejects one.
  --allow-rejected-options warn about rejected commands instead
  --cbhost PATH            cbhost executable (default: next to this program)

Openings:
  --openings DECK|FILE     acf156 (default: ACF 3-move deck without barred ballots),
                           acf174 (all ballots), acf144 (cross-board only, CheckerBoard's
                           default), or a file of FENs / move sequences / PDN games
  --rounds N               play the deck N times (default 1)
  --pairs N                stop after N opening pairs

Time control (CheckerBoard semantics, passed through getmove's maxtime/info/moreinfo):
  --movetime S             fixed time per move (default 0.1). Unused time is banked and
                           maxtime computed as in CheckerBoard engine matches. The bank is
                           charged the time the engine reports for its search (cbhost: the
                           getmove call), or the runner's wall time if it reports none.
  --movetime-mode cb|plain|exact
                           cb (default, as above); plain: maxtime = S every move;
                           exact: maxtime = S with CB_EXACT_TIME
  --tc INITIAL+INC         incremental time control in seconds, e.g. 60+0.5
  --no-time-forfeit        incremental TC: do not forfeit games on time
  --stop-factor F          fixed time: send stop after F*maxtime + grace (default 4)
  --grace S                default 0.25
  --kill-after S           kill an engine (loss) this long after stop (default 2)

Adjudication:
  --no-progress N          draw after N moves per side without a capture or man move
                           (default 40, CheckerBoard's rule; 0 = off)
  --max-plies N            adjudicate a draw after N plies (default 1000)
  --accept-draw-claims     end the game when the engine to move claims a draw (CheckerBoard
                           engine-match behaviour; off by default; not with --cb-claims)
  --cb-claims              CheckerBoard's claim handling with its default early adjudication:
                           after its move, an engine's draw claim ends the game as a draw, its
                           win claim as a win and its loss claim as a loss for it (a move that
                           leaves the opponent without a move keeps the board's result). Without
                           it claims change nothing; they are always recorded (PDN tags
                           FirstClaim, ClaimsA, ClaimsB; results.csv first_claim; with
                           --pdn-comments claim=... in the move's comment)
  --cb-move-limit N        CheckerBoard's limit of N search attempts per game (opening moves not
                           counted; 200 with early adjudication): once a game passes it, by the
                           engine move N + 1 or by finding the side to move without a move, it
                           ends with result * and no score, whatever that last attempt brought;
                           its pair is left out of the pair statistics

Running:
  -c, --concurrency N      games in parallel (default 6; 2 processes each)
  --sprt ELO0,ELO1[,ALPHA,BETA]
                           stop early when the GSPRT is decided (default alpha=beta=0.05).
                           With very few decisive pairs its error rates are not guaranteed:
                           a long run of drawn pairs can accept ELO0 too easily. Prefer a
                           fixed number of pairs for decisions.
  --sprt-min-pairs N       never stop before N complete pairs (default 20)
  -o, --out DIR            output directory (default matches/<timestamp>)
  --pdn-comments           add per-move time and engine output to the PDN
  --fresh-engines          start new engine processes for every game (default: each worker keeps
                           its two engines for all its games, as a GUI does; an engine may then
                           carry its hash table from game to game). Every PDN has the tags Worker,
                           EngineGamesA and EngineGamesB (games that engine's process had played
                           before).
  --pairs-together         a worker plays both games of an opening pair one after the other with
                           its two engines before it takes the next pair, as a GUI's engine match
                           does (default: each worker takes the next single game, so the two games
                           of a pair are usually played by different workers)
  -q, --quiet              no per-game lines
)");
}

bool parse_args(int argc, char** argv, Config& cfg) {
    cfg.spec[0].label = "A";
    cfg.spec[1].label = "B";
    std::string engine_arg[2];
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s needs %s\n", a.c_str(), what);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { usage(); std::exit(0); }
        else if (a == "-a") engine_arg[0] = need("an engine");
        else if (a == "-b") engine_arg[1] = need("an engine");
        else if (a == "--name-a") cfg.spec[0].name = need("a name");
        else if (a == "--name-b") cfg.spec[1].name = need("a name");
        else if (a == "--option-a") cfg.spec[0].cmds.push_back(need("a command"));
        else if (a == "--option-b") cfg.spec[1].cmds.push_back(need("a command"));
        else if (a == "--allow-rejected-options") cfg.spec[0].allow_rejected = cfg.spec[1].allow_rejected = true;
        else if (a == "--cbhost") cfg.cbhost = need("a path");
        else if (a == "--openings") cfg.deck = need("a deck");
        else if (a == "--rounds") cfg.rounds = std::atoi(need("N").c_str());
        else if (a == "--pairs") cfg.max_pairs = std::atol(need("N").c_str());
        else if (a == "--movetime") cfg.movetime = std::atof(need("seconds").c_str());
        else if (a == "--movetime-mode") {
            std::string m = need("cb|plain|exact");
            if (m == "cb") cfg.time_mode = TimeMode::CbMatch;
            else if (m == "plain") cfg.time_mode = TimeMode::Plain;
            else if (m == "exact") cfg.time_mode = TimeMode::Exact;
            else { std::fprintf(stderr, "bad --movetime-mode\n"); return false; }
        } else if (a == "--tc") {
            std::string t = need("INITIAL+INC");
            auto plus = t.find('+');
            cfg.tc_initial = std::atof(t.substr(0, plus).c_str());
            cfg.tc_increment = plus == std::string::npos ? 0 : std::atof(t.substr(plus + 1).c_str());
            cfg.time_mode = TimeMode::Incremental;
        } else if (a == "--no-time-forfeit") cfg.time_forfeit = false;
        else if (a == "--stop-factor") cfg.stop_factor = std::atof(need("F").c_str());
        else if (a == "--grace") cfg.grace = std::atof(need("seconds").c_str());
        else if (a == "--kill-after") cfg.kill_after = std::atof(need("seconds").c_str());
        else if (a == "--no-progress") cfg.no_progress = std::atoi(need("N").c_str());
        else if (a == "--max-plies") cfg.max_plies = std::atoi(need("N").c_str());
        else if (a == "--accept-draw-claims") cfg.accept_draw_claims = true;
        else if (a == "--cb-claims") cfg.cb_claims = true;
        else if (a == "--cb-move-limit") cfg.cb_move_limit = std::max(0, std::atoi(need("N").c_str()));
        else if (a == "-c" || a == "--concurrency") cfg.concurrency = std::max(1, std::atoi(need("N").c_str()));
        else if (a == "--sprt") {
            std::string s = need("ELO0,ELO1[,ALPHA,BETA]");
            double v[4] = {0, 5, 0.05, 0.05};
            int n = std::sscanf(s.c_str(), "%lf,%lf,%lf,%lf", &v[0], &v[1], &v[2], &v[3]);
            if (n < 2) { std::fprintf(stderr, "bad --sprt\n"); return false; }
            cfg.sprt = true;
            cfg.sprt_cfg.elo0 = v[0];
            cfg.sprt_cfg.elo1 = v[1];
            cfg.sprt_cfg.alpha = v[2];
            cfg.sprt_cfg.beta = v[3];
        } else if (a == "--sprt-min-pairs") {
            cfg.sprt_cfg.min_pairs = std::atol(need("N").c_str());
        } else if (a == "-o" || a == "--out") cfg.out_dir = need("a directory");
        else if (a == "--pdn-comments") cfg.pdn_comments = true;
        else if (a == "--fresh-engines") cfg.fresh_engines = true;
        else if (a == "--pairs-together") cfg.pairs_together = true;
        else if (a == "-q" || a == "--quiet") cfg.quiet = true;
        else {
            std::fprintf(stderr, "unknown option %s (see --help)\n", a.c_str());
            return false;
        }
    }
    if (engine_arg[0].empty() || engine_arg[1].empty()) {
        usage();
        return false;
    }
    // --accept-draw-claims takes a draw claim before the board's result, so with --cb-claims a capture that leaves the
    // opponent without a move would become a draw.
    if (cfg.cb_claims && cfg.accept_draw_claims) {
        std::fprintf(stderr, "--cb-claims and --accept-draw-claims cannot be combined: --cb-claims already ends the game "
                             "on a draw claim (after the board's result)\n");
        return false;
    }
    if (cfg.cbhost.empty()) cfg.cbhost = (fs::path(self_dir(argv[0])) / "cbhost").string();
    for (int e = 0; e < 2; ++e) {
        const std::string& s = engine_arg[e];
        if (s.rfind("exec:", 0) == 0) {
            cfg.spec[e].argv = split_command(s.substr(5));
        } else {
            if (!fs::exists(s)) {
                std::fprintf(stderr, "engine %s not found\n", s.c_str());
                return false;
            }
            cfg.spec[e].argv = {cfg.cbhost, fs::absolute(s).string()};
        }
        if (cfg.spec[e].argv.empty()) return false;
    }
    if (cfg.out_dir.empty()) {
        std::time_t t = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof buf, "matches/%Y%m%d-%H%M%S", std::localtime(&t));
        cfg.out_dir = buf;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    if (!parse_args(argc, argv, cfg)) return 2;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_sigint);

    Match m(cfg);
    std::string err;
    m.openings = load_openings(cfg.deck, err);
    if (m.openings.empty()) {
        std::fprintf(stderr, "openings: %s\n", err.c_str());
        return 2;
    }
    long pairs = long(m.openings.size()) * std::max(1, cfg.rounds);
    if (cfg.max_pairs > 0) pairs = std::min(pairs, cfg.max_pairs);
    m.total_games = 2 * pairs;

    fs::create_directories(fs::path(cfg.out_dir) / "games");
    fs::create_directories(fs::path(cfg.out_dir) / "logs");
    m.all_pdn.open(fs::path(cfg.out_dir) / "games.pdn");
    m.csv.open(fs::path(cfg.out_dir) / "results.csv");
    m.csv << "game,pair,opening,black,white,result,score_a,termination,engine_plies,first_claim\n";

    std::printf("match: %ld games (%zu openings from %s), %s, concurrency %d, output %s\n", m.total_games,
                m.openings.size(), cfg.deck.c_str(), tc_description(cfg).c_str(), cfg.concurrency, cfg.out_dir.c_str());
    std::fflush(stdout);

    int nworkers = int(std::min<long>(cfg.concurrency, m.total_games));
    std::vector<std::thread> threads;
    for (int i = 0; i < nworkers; ++i) threads.emplace_back(worker, std::ref(m), i);
    for (auto& t : threads) t.join();

    std::string summary = m.summary();
    std::printf("\n%s", summary.c_str());
    std::ofstream(fs::path(cfg.out_dir) / "summary.txt") << summary;
    return m.fatal.empty() ? 0 : 1;
}
