// Loss analysis: finds the Regicide move that lost each game and writes it to an avoid list.
//
//   lossprobe --pdn losses.pdn [--pdn more.pdn ...] --movetime 30000 --threads 8 --jobs 2 --egdb DIR
//             --out avoid.bin --report report.txt
//
// Regicide's side comes from the Black/White headers (a name containing --name, default "Regicide"), or --side.
// Only games Regicide lost are analysed. Each of its positions after the ballot (moves listed in the Opening header,
// else --ballot plies) is searched twice: once over all moves (best move and score) and once restricted to the move
// played, or, when that was the best move, to the other moves (best alternative). The walk stops at the first
// position whose best score is already lost (score <= -lost).
//
// Blame: the first Regicide move where
//   (a) the played move's score is at least --drop below the previous analysed Regicide position's score,
//   (b) another move scores at least --margin more than the played move, and
//   (c) the position before the move was not already lost.
// No such move: the loss is reported as unclear and nothing is blamed. Blamed (position, move) pairs go to --out as
// a book file holding only an avoid list (RGBK version 2); --merge adds an earlier file's entries (book and avoid).
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

struct Config {
    std::vector<std::string> pdns;
    std::string name = "Regicide";
    std::string side = "auto";  // auto | black | white
    Limits limits;
    int threads = 1;  // search threads per engine (Lazy SMP)
    int jobs = 1;     // games analysed in parallel
    std::size_t hash_mb = 256;
    int drop = 30;
    int margin = 20;
    int lost = 300;
    int ballot = 3;  // plies, when the Opening header lists no moves
    std::string egdb;
    std::string evalfile;
    std::string out = "avoid.bin";
    std::string report;
    std::string merge;
};

struct Game {
    std::string file;
    int index = 0;  // within the file, from 1
    std::map<std::string, std::string> tags;
    std::vector<std::string> moves;
};

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool is_result(const std::string& t) {
    return t == "1-0" || t == "0-1" || t == "1/2-1/2" || t == "*" || t == "2-0" || t == "0-2" || t == "1-1";
}

// Splits PDN text into games: tag pairs, then movetext with {comments} and (variations) removed.
std::vector<Game> read_pdn(const std::string& path) {
    std::ifstream in(path);
    std::vector<Game> games;
    if (!in) return games;
    Game g;
    g.file = path;
    bool in_moves = false;
    int comment = 0, variation = 0;
    auto finish = [&] {
        if (!g.tags.empty() || !g.moves.empty()) {
            g.index = int(games.size()) + 1;
            games.push_back(g);
        }
        g = Game{};
        g.file = path;
        in_moves = false;
    };
    for (std::string line; std::getline(in, line);) {
        std::size_t start = line.find_first_not_of(" \t\r");
        if (comment == 0 && variation == 0 && start != std::string::npos && line[start] == '[') {
            if (in_moves) finish();
            const std::size_t sp = line.find(' ', start), q1 = line.find('"', start), q2 = line.rfind('"');
            if (sp != std::string::npos && q1 != std::string::npos && q2 > q1)
                g.tags[line.substr(start + 1, sp - start - 1)] = line.substr(q1 + 1, q2 - q1 - 1);
            continue;
        }
        std::string token;
        auto flush = [&] {
            // Strip move numbers ("12." / "12...") glued to the token.
            std::size_t i = 0;
            while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) ++i;
            if (i < token.size() && token[i] == '.') {
                while (i < token.size() && token[i] == '.') ++i;
                token = token.substr(i);
            }
            if (!token.empty() && !is_result(token)) {
                g.moves.push_back(token);
                in_moves = true;
            } else if (is_result(token)) {
                in_moves = true;
            }
            token.clear();
        };
        for (char c : line) {
            if (c == '{') ++comment;
            else if (c == '}') comment = std::max(0, comment - 1);
            else if (comment) continue;
            else if (c == '(') ++variation;
            else if (c == ')') variation = std::max(0, variation - 1);
            else if (variation) continue;
            else if (std::isspace(static_cast<unsigned char>(c))) flush();
            else token += c;
            if (c == '{' || c == '(') flush();
        }
        flush();
    }
    finish();
    return games;
}

std::string score_str(int s) {
    char buf[32];
    if (s >= SCORE_MATE_BOUND) std::snprintf(buf, sizeof buf, "win/%d", SCORE_WIN - s);
    else if (s <= -SCORE_MATE_BOUND) std::snprintf(buf, sizeof buf, "loss/%d", SCORE_WIN + s);
    else if (s >= SCORE_KNOWN_WIN) std::snprintf(buf, sizeof buf, "dbwin/%d", SCORE_DB_WIN - s);
    else if (s <= -SCORE_KNOWN_WIN) std::snprintf(buf, sizeof buf, "dbloss/%d", SCORE_DB_WIN + s);
    else std::snprintf(buf, sizeof buf, "%+d", s);
    return buf;
}

std::string pv_str(const std::vector<Move>& pv, std::size_t n = 8) {
    std::string s;
    for (std::size_t i = 0; i < pv.size() && i < n; ++i) s += (i ? " " : "") + move_to_string(pv[i]);
    return s;
}

struct Blame {
    Position pos;
    Move move;
    int score = 0;
    int depth = 0;
};

struct Outcome {
    std::string text;
    std::vector<Blame> blames;  // at most one
    bool analysed = false, unclear = false, error = false;
};

Outcome analyse(Engine& engine, const Game& g, const Config& cfg) {
    Outcome out;
    std::ostringstream r;
    auto tag = [&](const std::string& k) {
        auto it = g.tags.find(k);
        return it == g.tags.end() ? std::string() : it->second;
    };
    r << "== " << g.file << " #" << g.index << ": " << tag("Black") << " (black) - " << tag("White") << " (white)  "
      << tag("Result") << "  " << tag("Opening") << "  round " << tag("Round") << '\n';

    Color us;
    const bool black = lower(tag("Black")).find(lower(cfg.name)) != std::string::npos;
    const bool white = lower(tag("White")).find(lower(cfg.name)) != std::string::npos;
    if (cfg.side == "black" || cfg.side == "white") us = cfg.side == "black" ? BLACK : WHITE;
    else if (black != white) us = black ? BLACK : WHITE;
    else {
        r << "   skipped: cannot tell which side " << cfg.name << " played (use --side)\n";
        out.text = r.str();
        return out;
    }
    const std::string result = tag("Result");
    if (result != (us == BLACK ? "0-1" : "1-0")) {
        r << "   skipped: not a loss for " << (us == BLACK ? "black" : "white") << '\n';
        out.text = r.str();
        return out;
    }

    // Ballot length: the moves after the colon in the Opening header, else --ballot.
    int ballot = cfg.ballot;
    if (const std::string op = tag("Opening"); op.find(':') != std::string::npos) {
        std::istringstream in(op.substr(op.find(':') + 1));
        int n = 0;
        for (std::string t; in >> t;) n += t.find_first_of("-x") != std::string::npos;
        if (n > 0) ballot = n;
    }

    Position pos;
    if (const std::string fen = tag("FEN"); !fen.empty() && !pos.set_fen(fen)) {
        r << "   error: bad FEN " << fen << '\n';
        out.text = r.str();
        out.error = true;
        return out;
    }
    std::vector<Position> positions{pos};
    std::vector<Move> moves;
    for (const std::string& t : g.moves) {
        Move m;
        if (!parse_move(positions.back(), t, m)) {
            r << "   error: illegal move " << t << " at ply " << moves.size() << " in " << positions.back().fen()
              << '\n';
            out.text = r.str();
            out.error = true;
            return out;
        }
        moves.push_back(m);
        Position next = positions.back();
        next.make_move(m);
        positions.push_back(next);
    }
    out.analysed = true;
    r << "   " << (us == BLACK ? "black" : "white") << ", ballot " << ballot << " plies, " << moves.size()
      << " plies played\n";
    r << "   ply  move  played            best/alternative           depth  pv (best)\n";

    engine.new_game();
    const int first_move = positions[0].side_to_move() == BLACK ? 0 : 1;
    auto move_no = [&](std::size_t ply) {
        const std::size_t p = ply + first_move;
        return std::to_string(p / 2 + 1) + (p % 2 ? "..." : ".");
    };
    std::optional<int> prev;
    bool done = false;
    for (std::size_t ply = std::size_t(ballot); ply < moves.size() && !done; ++ply) {
        const Position& p = positions[ply];
        if (p.side_to_move() != us) continue;
        MoveList list;
        generate_moves(p, list);
        const Move played = moves[ply];
        char head[64];
        std::snprintf(head, sizeof head, "   %3zu  %-5s %-8s", ply, move_no(ply).c_str(),
                      move_to_string(played).c_str());
        if (list.size() == 1) {
            r << head << "forced\n";
            continue;
        }
        const std::vector<Position> history(positions.begin(), positions.begin() + std::ptrdiff_t(ply));

        Limits limits = cfg.limits;
        limits.use_book = false;
        limits.searchmoves.assign(list.begin(), list.end());
        engine.set_position(p, history);
        // A losing score from an unfinished iteration is no verdict on the position (SearchResult::sound_score).
        SearchResult full = engine.go(limits);
        full.score = full.sound_score();

        // Second search: the played move alone, or every other move when the played move was the best.
        const bool played_best = full.best == played;
        limits.searchmoves.clear();
        for (const Move& m : list)
            if ((m == played) != played_best) limits.searchmoves.push_back(m);
        engine.set_position(p, history);
        SearchResult second = engine.go(limits);
        second.score = second.sound_score();
        const SearchResult& played_r = played_best ? full : second;
        const SearchResult& alt_r = played_best ? second : full;
        const int best_score = std::max(full.score, second.score);

        char line[256];
        std::snprintf(line, sizeof line, "%-8s %-8s %-6s %-8s %3d/%-3d  %s", score_str(played_r.score).c_str(),
                      played_best ? "(best)" : "", move_to_string(alt_r.best).c_str(),
                      score_str(alt_r.score).c_str(), full.depth, second.depth, pv_str(full.pv).c_str());
        r << head << line << '\n';

        if (best_score <= -cfg.lost) {
            r << "   already lost at ply " << ply << " (best " << score_str(best_score) << ")\n";
            break;
        }
        const int before = prev.value_or(best_score);
        if (played_r.score <= before - cfg.drop && alt_r.score >= played_r.score + cfg.margin) {
            r << "   BLAME ply " << ply << " " << move_no(ply) << " " << move_to_string(played) << "  FEN " << p.fen()
              << "\n     played " << move_to_string(played) << " " << score_str(played_r.score) << " (depth "
              << played_r.depth << ", pv " << pv_str(played_r.pv) << ")\n     best alternative "
              << move_to_string(alt_r.best) << " " << score_str(alt_r.score) << " (depth " << alt_r.depth << ", pv "
              << pv_str(alt_r.pv) << ")\n     drop " << before - played_r.score << " from the previous score "
              << score_str(before) << ", margin " << alt_r.score - played_r.score << '\n';
            out.blames.push_back({p, played, played_r.score, played_r.depth});
            done = true;
        }
        prev = best_score;
    }
    if (out.blames.empty()) {
        out.unclear = true;
        r << "   UNCLEAR: no move meets the blame rule\n";
    }
    out.text = r.str();
    return out;
}

void usage() {
    std::cout << "usage: lossprobe --pdn FILE [--pdn FILE ...] [--nodes N | --movetime MS | --depth D]\n"
                 "                 [--threads T] [--jobs J] [--hash MB] [--egdb DIR] [--evalfile F]\n"
                 "                 [--name Regicide] [--side auto|black|white] [--ballot 3]\n"
                 "                 [--drop 30] [--margin 20] [--lost 300]\n"
                 "                 [--out avoid.bin] [--report report.txt] [--merge old.bin]\n";
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    cfg.limits.nodes = 1000000;
    for (int i = 1; i < argc; ++i) {
        const std::string k = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << k << " needs a value\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (k == "--pdn" || k == "--pgn") cfg.pdns.push_back(next());
        else if (k == "--nodes") cfg.limits.nodes = std::stoull(next());
        else if (k == "--movetime") cfg.limits.movetime_ms = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--depth") cfg.limits.depth = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--threads") cfg.threads = std::max(1, std::stoi(next()));
        else if (k == "--jobs") cfg.jobs = std::max(1, std::stoi(next()));
        else if (k == "--hash") cfg.hash_mb = std::stoull(next());
        else if (k == "--egdb") cfg.egdb = next();
        else if (k == "--evalfile") cfg.evalfile = next();
        else if (k == "--name") cfg.name = next();
        else if (k == "--side") cfg.side = next();
        else if (k == "--ballot") cfg.ballot = std::stoi(next());
        else if (k == "--drop") cfg.drop = std::stoi(next());
        else if (k == "--margin") cfg.margin = std::stoi(next());
        else if (k == "--lost") cfg.lost = std::stoi(next());
        else if (k == "--out") cfg.out = next();
        else if (k == "--report") cfg.report = next();
        else if (k == "--merge") cfg.merge = next();
        else if (k == "-h" || k == "--help") return usage(), 0;
        else if (!k.empty() && k[0] != '-') cfg.pdns.push_back(k);
        else {
            std::cerr << "unknown option " << k << '\n';
            usage();
            return 2;
        }
    }
    if (cfg.pdns.empty()) return usage(), 2;

    std::shared_ptr<const Evaluator> ev = Evaluator::builtin();
    if (!cfg.evalfile.empty() && !(ev = Evaluator::from_file(cfg.evalfile))) {
        std::cerr << "cannot load " << cfg.evalfile << '\n';
        return 1;
    }
    int egdb_pieces = 0;
    if (cfg.egdb.empty())
        if (const char* env = std::getenv("REGICIDE_EGDB")) cfg.egdb = env;
    if (!cfg.egdb.empty()) {
        if (!egdb::init(cfg.egdb, 4096)) {
            std::cerr << "no endgame database in " << cfg.egdb << '\n';
            return 1;
        }
        egdb_pieces = egdb::max_pieces();
    }

    std::vector<Game> games;
    for (const std::string& f : cfg.pdns) {
        std::vector<Game> gs = read_pdn(f);
        if (gs.empty()) std::cerr << "warning: no games in " << f << '\n';
        games.insert(games.end(), gs.begin(), gs.end());
    }
    std::ostringstream head;
    head << "lossprobe: " << games.size() << " games;";
    if (cfg.limits.nodes) head << " nodes " << cfg.limits.nodes;
    if (cfg.limits.movetime_ms) head << " movetime " << cfg.limits.movetime_ms << " ms";
    if (cfg.limits.depth < MAX_PLY - 1) head << " depth " << cfg.limits.depth;
    head << ", threads " << cfg.threads << ", jobs " << cfg.jobs << ", egdb " << egdb_pieces << " pieces, eval "
         << (ev->network() ? "nnue" : ev->trained() ? "linear" : "hce") << ", drop " << cfg.drop << ", margin "
         << cfg.margin << ", lost " << cfg.lost << '\n';
    std::cout << head.str() << std::flush;

    std::vector<Outcome> outcomes(games.size());
    std::atomic<std::size_t> next{0};
    std::mutex mu;
    std::vector<std::thread> pool;
    for (int t = 0; t < std::min<int>(cfg.jobs, int(games.size())); ++t)
        pool.emplace_back([&] {
            Engine engine;
            engine.set_hash_mb(cfg.hash_mb);
            engine.set_threads(cfg.threads);
            engine.set_evaluator(ev);
            engine.set_egdb_pieces(egdb_pieces);
            for (std::size_t i; (i = next++) < games.size();) {
                outcomes[i] = analyse(engine, games[i], cfg);
                std::lock_guard lock(mu);
                std::cout << outcomes[i].text << std::flush;
            }
        });
    for (auto& th : pool) th.join();

    Book avoid;
    if (!cfg.merge.empty()) {
        if (!avoid.load(cfg.merge)) {
            std::cerr << "cannot load " << cfg.merge << '\n';
            return 1;
        }
    }
    const std::size_t merged = avoid.avoid_size();
    Book blames;
    int analysed = 0, blamed = 0, unclear = 0, errors = 0;
    std::ostringstream report;
    report << head.str();
    for (const Outcome& o : outcomes) {
        report << o.text;
        analysed += o.analysed;
        unclear += o.unclear;
        errors += o.error;
        for (const Blame& b : o.blames) {
            blames.add_avoid(Book::make_entry(b.pos, b.move, b.score, b.depth));
            ++blamed;
        }
    }
    // The same move blamed in several games: one entry counting them.
    std::vector<Book::Entry> run(blames.avoid_entries());
    std::ranges::sort(run, {}, [](const Book::Entry& e) { return std::tuple(e.key, e.from, e.to, e.disc); });
    for (std::size_t i = 0; i < run.size();) {
        std::size_t j = i;
        Book::Entry e = run[i];
        for (; j < run.size() && run[j].key == e.key && run[j].from == e.from && run[j].to == e.to &&
               run[j].disc == e.disc;
             ++j)
            if (run[j].depth > e.depth) e = run[j];
        e.count = std::uint16_t(j - i);
        avoid.add_avoid(e);
        i = j;
    }
    avoid.finalize();
    std::ostringstream summary;
    summary << "summary: " << games.size() << " games, " << analysed << " losses analysed, " << blamed
            << " blamed, " << unclear << " unclear, " << errors << " errors; " << cfg.out << ": "
            << avoid.avoid_size() << " avoid entries (" << merged << " merged from " << (cfg.merge.empty() ? "-" : cfg.merge)
            << ")\n";
    report << summary.str();
    std::cout << summary.str();
    if (!avoid.save(cfg.out)) {
        std::cerr << "cannot write " << cfg.out << '\n';
        return 1;
    }
    if (!cfg.report.empty()) {
        std::ofstream rf(cfg.report);
        rf << report.str();
        if (!rf) {
            std::cerr << "cannot write " << cfg.report << '\n';
            return 1;
        }
    }
    return 0;
}
