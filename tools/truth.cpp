// Endgame-database truth for training data (DataRecord files written by datagen).
//
//   truth census  --egdb DIR FILE...                 piece-count histogram, games, database vs game result
//   truth relabel --egdb DIR --in IN --out OUT       game results := database value of the game's first record
//                                                    with <= database pieces (games that never get there keep
//                                                    theirs); records are copied otherwise unchanged
//   truth pilot   --egdb DIR --n 200000 --min 9 --max 14 --nodes 10000,50000,200000 --threads 8 --seed 1
//                 --out pilot.csv FILE...            searches random records with the database at each budget
//   truth split   --in IN --out OUT --min 9 --max 12 --proven [--from F]   copies matching records (F: first
//                                                    record index considered)
//   truth probe   --egdb DIR FEN...                  database value (side to move) of each position
//   truth losswalk --egdb DIR --nodes 20000000 --threads 6 [--opponents NAME] --out lost.tsv PDN...
//                                                    where each game NAME (default regicide) lost was decided
//   truth seeds   --min 6 --max 40 --opponents cake,kingsrow,guinn --out seeds.txt PDN...
//                                                    positions after plies min..max of real games (datagen --seeds)
//
// Game boundaries. Files written by datagen since 2026-11-10 mark the first record of every game
// (DataRecord::GAME_START) and are split there. For older files: datagen appends whole games, one at a time, in play order, so a file is a sequence of games. Two
// consecutive records belong to the same game unless the second could not follow the first: a different game result,
// more pieces of either colour, or a man that moved backwards (for every row r, the number of black men on rows >= r
// never grows, and the number of white men on rows <= r never grows: men only move forward or disappear).
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "datafile.hpp"
#include "egdb/egdb.hpp"
#include "label.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

bool same_game(const DataRecord& a, const DataRecord& b) {
    if (a.result != b.result) return false;
    if (popcount(b.black) > popcount(a.black) || popcount(b.white) > popcount(a.white)) return false;
    const Bitboard abm = a.black & ~a.kings, bbm = b.black & ~b.kings;
    const Bitboard awm = a.white & ~a.kings, bwm = b.white & ~b.kings;
    for (int r = 0; r < 8; ++r) {
        const Bitboard up = ~Bitboard(0) << (4 * r);  // rows >= r
        if (popcount(bbm & up) > popcount(abm & up)) return false;
        const Bitboard down = r == 7 ? ~Bitboard(0) : (Bitboard(1) << (4 * (r + 1))) - 1;  // rows <= r
        if (popcount(bwm & down) > popcount(awm & down)) return false;
    }
    return true;
}

// Database value from black's point of view: 1 / 0 / -1, or 2 when unknown.
int db_black(const DataRecord& r) {
    const egdb::Wld w = egdb::probe(r.black, r.white, r.kings, r.color());
    if (w == egdb::Wld::Unknown) return 2;
    const int s = w == egdb::Wld::Win ? 1 : w == egdb::Wld::Loss ? -1 : 0;
    return r.color() == BLACK ? s : -s;
}

struct Reader {
    std::FILE* f = nullptr;
    std::vector<DataRecord> buf;
    std::size_t pos = 0, len = 0;
    explicit Reader(const std::string& path) : f(std::fopen(path.c_str(), "rb")), buf(1 << 20) {}
    ~Reader() {
        if (f) std::fclose(f);
    }
    bool next(DataRecord& r) {
        if (pos == len) {
            len = f ? std::fread(buf.data(), sizeof(DataRecord), buf.size(), f) : 0;
            pos = 0;
            if (len == 0) return false;
        }
        r = buf[pos++];
        return true;
    }
};

// Calls on_game(records) for every game of the file. Records flagged DataRecord::GAME_START (datagen since
// 2026-11-10) start a game. Up to the first flagged record (a legacy file, or a legacy prefix that flagged games were
// appended to) games are split by the same_game() heuristic; from the first flag on, by the flags only. `flagged`, if
// given, is set when the file has any flag.
template <class F>
long for_each_game(const std::string& path, F on_game, bool* flagged = nullptr) {
    Reader in(path);
    if (!in.f) {
        std::cerr << "cannot open " << path << '\n';
        std::exit(1);
    }
    std::vector<DataRecord> game;
    DataRecord r;
    long games = 0;
    bool seen_flag = false;
    if (flagged) *flagged = false;
    while (in.next(r)) {
        if (r.game_start() && !seen_flag) {
            seen_flag = true;
            if (flagged) *flagged = true;
        }
        const bool boundary = r.game_start() || (!seen_flag && !game.empty() && !same_game(game.back(), r));
        if (!game.empty() && boundary) {
            on_game(game);
            ++games;
            game.clear();
        }
        game.push_back(r);
    }
    if (!game.empty()) {
        on_game(game);
        ++games;
    }
    return games;
}

std::string base(const std::string& p) {
    const auto s = p.find_last_of('/');
    return s == std::string::npos ? p : p.substr(s + 1);
}

int census(const std::vector<std::string>& files) {
    const int db = egdb::max_pieces();
    std::printf("# database: %d pieces\n", db);
    for (const auto& file : files) {
        std::array<long, 25> hist{};
        long records = 0, games = 0, reach = 0, reach_records_9 = 0, changed_records_9 = 0, unknown = 0;
        long m[3][3] = {};  // [database +1][stored result +1], black's view
        long len_sum = 0;
        bool flagged = false;
        for_each_game(file, [&](const std::vector<DataRecord>& g) {
            ++games;
            len_sum += long(g.size());
            for (const auto& r : g) hist[r.pieces()]++;
            records += long(g.size());
            const auto it = std::find_if(g.begin(), g.end(), [&](const DataRecord& r) { return r.pieces() <= db; });
            if (it == g.end()) return;
            const int v = db_black(*it);
            if (v == 2) {
                ++unknown;
                return;
            }
            ++reach;
            m[v + 1][g[0].result + 1]++;
            const long n9 = std::count_if(g.begin(), g.end(), [&](const DataRecord& r) { return r.pieces() > db; });
            reach_records_9 += n9;
            if (v != g[0].result) changed_records_9 += n9;
        }, &flagged);
        long low = 0;
        for (int p = 0; p <= db; ++p) low += hist[p];
        std::printf("file %s records %ld games %ld avg_len %.1f le%d %ld (%.2f%%)\n", base(file).c_str(), records,
                    games, double(len_sum) / std::max(1L, games), db, low, 100.0 * low / std::max(1L, records));
        std::printf("boundaries %s %s\n", base(file).c_str(), flagged ? "flags" : "heuristic");
        std::printf("hist %s", base(file).c_str());
        for (int p = 1; p <= 24; ++p) std::printf(" %ld", hist[p]);
        std::printf("\n");
        long agree = m[0][0] + m[1][1] + m[2][2];
        std::printf("games %s reach_le%d %ld (%.1f%%) unknown %ld disagree %ld (%.2f%%) changed_ge%d_records %ld of "
                    "%ld in those games (%.2f%% of all ge%d)\n",
                    base(file).c_str(), db, reach, 100.0 * reach / std::max(1L, games), unknown, reach - agree,
                    100.0 * (reach - agree) / std::max(1L, reach), db + 1, changed_records_9, reach_records_9,
                    100.0 * changed_records_9 / std::max(1L, records - low), db + 1);
        std::printf("matrix %s db\\stored(L D W):", base(file).c_str());
        for (int d = 0; d < 3; ++d) std::printf("  %ld %ld %ld |", m[d][0], m[d][1], m[d][2]);
        std::printf("\n");
        std::fflush(stdout);
    }
    return 0;
}

int relabel(const std::string& in, const std::string& out) {
    const int db = egdb::max_pieces();
    std::FILE* o = std::fopen(out.c_str(), "wb");
    if (!o) {
        std::cerr << "cannot open " << out << '\n';
        return 1;
    }
    long games = 0, changed = 0, records = 0;
    bool flagged = false;
    for_each_game(in, [&](std::vector<DataRecord>& g) {
        const auto it = std::find_if(g.begin(), g.end(), [&](const DataRecord& r) { return r.pieces() <= db; });
        if (it != g.end()) {
            const int v = db_black(*it);
            if (v != 2 && v != g[0].result) {
                ++changed;
                for (auto& r : g) r.result = std::int8_t(v);
            }
        }
        ++games;
        records += long(g.size());
        std::fwrite(g.data(), sizeof(DataRecord), g.size(), o);
    }, &flagged);
    std::fclose(o);
    std::printf("relabel %s -> %s: records %ld games %ld (boundaries: %s) relabelled %ld\n", base(in).c_str(),
                base(out).c_str(), records, games, flagged ? "flags" : "heuristic", changed);
    return 0;
}

// Uniform random records with min..max pieces from the files (by global index), searched at each node budget.
int pilot(const std::vector<std::string>& files, long n, int lo, int hi, const std::vector<std::uint64_t>& budgets,
          int threads, std::uint64_t seed, const std::string& out) {
    struct Src {
        std::string name;
        std::FILE* f;
        long count;
    };
    std::vector<Src> src;
    long total = 0;
    for (const auto& file : files) {
        std::FILE* f = std::fopen(file.c_str(), "rb");
        if (!f) return std::cerr << "cannot open " << file << '\n', 1;
        std::fseek(f, 0, SEEK_END);
        const long c = std::ftell(f) / 16;
        src.push_back({base(file), f, c});
        total += c;
    }
    struct Item {
        int file;
        long index;
        DataRecord rec;
    };
    std::vector<Item> items;
    std::mt19937_64 rng(seed);
    if (total <= 0) return std::cerr << "the input holds no complete record\n", 1;
    for (long tries = 0; long(items.size()) < n; ++tries) {
        if (tries > 1000 * n + 1000000)
            return std::cerr << "too few records with " << lo << ".." << hi << " pieces in the input\n", 1;
        long g = long(rng() % std::uint64_t(total));
        int fi = 0;
        while (g >= src[fi].count) g -= src[fi++].count;
        DataRecord r;
        std::fseek(src[fi].f, g * 16, SEEK_SET);
        if (std::fread(&r, 16, 1, src[fi].f) != 1) return 1;
        if (r.pieces() < lo || r.pieces() > hi) continue;
        items.push_back({fi, g, r});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        return a.file != b.file ? a.file < b.file : a.index < b.index;
    });
    std::vector<std::vector<int>> scores(items.size(), std::vector<int>(budgets.size()));
    std::atomic<long> next{0}, done{0};
    auto worker = [&] {
        Engine engine;
        engine.set_hash_mb(32);
        engine.set_egdb_pieces(egdb::max_pieces());
        for (long i; (i = next++) < long(items.size());) {
            for (std::size_t b = 0; b < budgets.size(); ++b) {
                engine.new_game();
                engine.set_position(items[i].rec.position());
                Limits l;
                l.nodes = budgets[b];
                l.use_book = false;
                const SearchResult sr = engine.go(l);
                // label.hpp, as rescore and datagen: a partial losing score falls back to the completed iteration's score;
                // a search without a completed iteration counts as unknown (0), never as a proof
                const auto lab = label_of(sr);
                scores[i][b] = lab ? lab->score : 0;
            }
            const long d = ++done;
            if (d % 10000 == 0) {
                std::printf("pilot %ld / %zu\n", d, items.size());
                std::fflush(stdout);
            }
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    std::FILE* o = std::fopen(out.c_str(), "w");
    std::fprintf(o, "file,index,pieces,stm,stored_score,result_black,db_first_le8_unused");
    for (auto b : budgets) std::fprintf(o, ",s%llu", (unsigned long long)b);
    std::fprintf(o, "\n");
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& it = items[i];
        std::fprintf(o, "%s,%ld,%d,%d,%d,%d,0", src[it.file].name.c_str(), it.index, it.rec.pieces(),
                     int(it.rec.color()), it.rec.score, it.rec.result);
        for (int s : scores[i]) std::fprintf(o, ",%d", s);
        std::fprintf(o, "\n");
    }
    std::fclose(o);
    for (auto& s : src) std::fclose(s.f);
    return 0;
}

// ---- seeds: positions from real games (PDN) for datagen --seeds

bool is_result(const std::string& t) {
    return t == "1-0" || t == "0-1" || t == "1/2-1/2" || t == "*" || t == "2-0" || t == "0-2" || t == "1-1";
}

struct PdnGame {
    std::map<std::string, std::string> tags;
    std::vector<std::string> moves;
    std::vector<std::string> comments;  // comments[i]: the {comment} text after moves[i] ("" if none)
};

// Tag pairs and movetext with {comments} and (variations) removed (as tools/lossprobe.cpp).
std::vector<PdnGame> read_pdn(const std::string& path) {
    std::ifstream in(path);
    std::vector<PdnGame> games;
    PdnGame g;
    bool in_moves = false;
    int comment = 0, variation = 0;
    auto finish = [&] {
        if (!g.tags.empty() || !g.moves.empty()) games.push_back(g);
        g = PdnGame{};
        in_moves = false;
    };
    for (std::string line; std::getline(in, line);) {
        const std::size_t start = line.find_first_not_of(" \t\r");
        if (comment == 0 && variation == 0 && start != std::string::npos && line[start] == '[') {
            if (in_moves) finish();
            const std::size_t sp = line.find(' ', start), q1 = line.find('"', start), q2 = line.rfind('"');
            if (sp != std::string::npos && q1 != std::string::npos && q2 > q1)
                g.tags[line.substr(start + 1, sp - start - 1)] = line.substr(q1 + 1, q2 - q1 - 1);
            continue;
        }
        std::string token;
        auto flush = [&] {
            std::size_t i = 0;
            while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) ++i;
            if (i < token.size() && token[i] == '.') {
                while (i < token.size() && token[i] == '.') ++i;
                token = token.substr(i);
            }
            if (!token.empty()) {
                if (!is_result(token)) {
                    g.moves.push_back(token);
                    g.comments.emplace_back();
                }
                in_moves = true;
            }
            token.clear();
        };
        // a comment continued from the previous line (main line only, as below)
        if (comment && !variation && !g.comments.empty()) g.comments.back() += ' ';
        for (char c : line) {
            if (c == '{') ++comment;
            else if (c == '}') comment = std::max(0, comment - 1);
            else if (comment) {
                // only main-line comments belong to the last main-line move; a comment inside a (variation) does not
                if (!variation && !g.comments.empty()) g.comments.back() += c;
                continue;
            }
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

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Positions after plies lo..hi of every game in which one player's name contains one of `opponents` (lowercase,
// comma separated; empty = every game), deduplicated, one FEN per line.
int seeds(const std::vector<std::string>& files, int lo, int hi, const std::string& opponents, const std::string& out) {
    std::vector<std::string> opp;
    for (std::size_t p = 0; p < opponents.size();) {
        const std::size_t c = std::min(opponents.find(',', p), opponents.size());
        if (c > p) opp.push_back(opponents.substr(p, c - p));
        p = c + 1;
    }
    std::set<std::string> fens;
    long games = 0, used = 0, bad = 0;
    std::map<std::string, long> by_opp;
    for (const auto& file : files) {
        for (const auto& g : read_pdn(file)) {
            ++games;
            const auto tag = [&](const char* k) {
                const auto it = g.tags.find(k);
                return it == g.tags.end() ? std::string() : lower(it->second);
            };
            std::string who;
            for (const auto& o : opp)
                if (tag("Black").find(o) != std::string::npos || tag("White").find(o) != std::string::npos) who = o;
            if (!opp.empty() && who.empty()) continue;
            Position pos;
            if (const std::string fen = g.tags.count("FEN") ? g.tags.at("FEN") : ""; !fen.empty() && !pos.set_fen(fen)) {
                ++bad;
                continue;
            }
            ++used;
            by_opp[who]++;
            for (int ply = 0; ply < int(g.moves.size()) && ply < hi; ++ply) {
                Move m;
                if (!parse_move(pos, g.moves[ply], m)) {
                    ++bad;
                    break;
                }
                pos.make_move(m);
                if (ply + 1 >= lo) fens.insert(pos.fen());
            }
        }
    }
    std::ofstream o(out);
    for (const auto& f : fens) o << f << '\n';
    std::printf("seeds: %ld games read, %ld used (", games, used);
    for (const auto& [k, v] : by_opp) std::printf(" %s %ld", k.c_str(), v);
    std::printf(" ), %ld with errors, %zu distinct positions after plies %d..%d -> %s\n", bad, fens.size(), lo, hi,
                out.c_str());
    return 0;
}

// A move written by its two end squares only ("2x18") is ambiguous when more than one legal move runs from the first to
// the last square (different capture paths taking different pieces). parse_move would take the first match.
bool ambiguous_capture(const Position& pos, const std::string& text) {
    auto squares = [](const std::string& s) {
        std::vector<int> v;
        int n = -1;
        for (char ch : s + '-') {
            if (std::isdigit(static_cast<unsigned char>(ch))) n = (n < 0 ? 0 : n * 10) + (ch - '0');
            else {
                if (n > 0) v.push_back(n);
                n = -1;
            }
        }
        return v;
    };
    const std::vector<int> sq = squares(text);
    if (sq.size() != 2) return false;
    MoveList list;
    generate_moves(pos, list);
    // The parsers' identity rule (PR #38): two squares one jump apart mean that single jump when it is legal; otherwise
    // a capture is identified by the set of pieces it takes, and only different sets are ambiguous.
    std::vector<Bitboard> taken;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const std::vector<int> v = squares(move_to_string(list[i]));
        if (v.size() < 2 || v.front() != sq[0] || v.back() != sq[1]) continue;
        if (popcount(list[i].captures) == 1) return false;  // the single jump itself is legal
        if (std::find(taken.begin(), taken.end(), list[i].captures) == taken.end()) taken.push_back(list[i].captures);
    }
    return taken.size() > 1;
}

// ---- losswalk: where were the games Regicide lost decided?
//
// For every game a player whose name contains `name` lost, the positions after each of its moves are searched (from
// the last move backwards) with `nodes` nodes and the database; positions with <= database pieces take the database
// value. The losing move is the first move of the final run of moves after which the position is proven won for the
// opponent (label.hpp: a win is a proof). One TSV row per game.
int losswalk(const std::vector<std::string>& files, const std::string& name, std::uint64_t nodes, int threads,
             const std::string& out) {
    struct Job {
        std::string file;
        PdnGame g;
        Color us;
    };
    std::vector<Job> jobs;
    for (const auto& f : files)
        for (auto& g : read_pdn(f)) {
            const auto tag = [&](const char* k) {
                const auto it = g.tags.find(k);
                return it == g.tags.end() ? std::string() : it->second;
            };
            const bool black = lower(tag("Black")).find(lower(name)) != std::string::npos;
            const bool white = lower(tag("White")).find(lower(name)) != std::string::npos;
            if (black == white || tag("Result") != (black ? "0-1" : "1-0")) continue;
            jobs.push_back({f, g, black ? BLACK : WHITE});
        }
    std::vector<std::string> rows(jobs.size());
    std::atomic<std::size_t> next{0};
    auto worker = [&] {
        Engine engine;
        engine.set_hash_mb(256);
        engine.set_egdb_pieces(egdb::max_pieces());
        for (std::size_t j; (j = next++) < jobs.size();) {
            const Job& job = jobs[j];
            const PdnGame& g = job.g;
            const auto tag = [&](const char* k) {
                const auto it = g.tags.find(k);
                return it == g.tags.end() ? std::string() : it->second;
            };
            Position pos;
            std::string error;
            if (g.tags.count("FEN") && !pos.set_fen(g.tags.at("FEN"))) error = "PARSE-ERROR bad FEN";
            const Color first_mover = pos.side_to_move();
            std::vector<Position> before, after;
            std::vector<std::size_t> ply;
            bool ok = error.empty();
            for (std::size_t i = 0; ok && i < g.moves.size(); ++i) {
                Move m;
                if (!parse_move(pos, g.moves[i], m)) {
                    ok = false;
                    error = "PARSE-ERROR illegal move " + g.moves[i] + " at ply " + std::to_string(i);
                    break;
                }
                // A capture written by its end squares only is ambiguous when two different captures share them.
                if (ambiguous_capture(pos, g.moves[i])) {
                    ok = false;
                    error = "UNRESOLVED ambiguous capture " + g.moves[i] + " at ply " + std::to_string(i);
                    break;
                }
                const bool ours = pos.side_to_move() == job.us;
                const Position b = pos;
                pos.make_move(m);
                if (ours) {
                    before.push_back(b);
                    after.push_back(pos);
                    ply.push_back(i);
                }
            }
            // Value of a position for its side to move: (proven, score); <= database pieces: the database.
            auto value = [&](const Position& p, std::string* best) -> std::pair<bool, int> {
                if (p.piece_count() <= egdb::max_pieces()) {
                    const egdb::Wld w = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
                    if (w != egdb::Wld::Unknown)
                        return {w != egdb::Wld::Draw, w == egdb::Wld::Win ? SCORE_DB_WIN : w == egdb::Wld::Loss ? -SCORE_DB_WIN : 0};
                }
                engine.new_game();
                engine.set_position(p);
                Limits l;
                l.nodes = nodes;
                l.use_book = false;
                const SearchResult sr = engine.go(l);
                if (best) *best = move_to_string(sr.best);
                const auto lab = label_of(sr);
                return lab ? std::pair{lab->proven, lab->score} : std::pair{false, 0};
            };
            std::string row = job.file + "\t" + tag("Black") + "\t" + tag("White") + "\t" + tag("Round") + "\t" +
                              tag("Opening") + "\t" + (job.us == BLACK ? "black" : "white") + "\t" + tag("Termination");
            if (!ok || after.empty()) {
                rows[j] = row + "\t" + (error.empty() ? std::string("PARSE-ERROR no moves") : error);
                continue;
            }
            int k = int(after.size()) - 1, scanned = 0;
            std::vector<int> opp_score(after.size(), 0);
            for (; k >= 0; --k) {
                const auto [proven, s] = value(after[std::size_t(k)], nullptr);
                ++scanned;
                opp_score[std::size_t(k)] = s;
                if (!(proven && s > 0)) break;
            }
            if (k == int(after.size()) - 1) {
                rows[j] = row + "\tEND-NOT-PROVEN\t" + std::to_string(opp_score[std::size_t(k)]);
                continue;
            }
            const std::size_t b = std::size_t(k + 1);
            std::string best;
            const auto [bproven, bscore] = value(before[b], &best);
            // The turning move: walk back over Regicide's decision points from b until the deep score (side to move =
            // Regicide) is >= -50; the move played there is where the game turned from about level to bad.
            auto ingame = [&](std::size_t p) {
                const std::string& c = g.comments[p];
                const std::size_t at = c.find("score ");
                if (at != std::string::npos) return std::atoi(c.c_str() + at + 6);
                const std::size_t v = c.find("value=");
                return v != std::string::npos ? std::atoi(c.c_str() + v + 6) : 99999;
            };
            std::string traj;
            long t = long(b);
            int tscore = bscore;
            for (;;) {
                char tb[64];
                std::snprintf(tb, sizeof tb, "%s%zu:%d:%d", traj.empty() ? "" : " ", ply[std::size_t(t)],
                              before[std::size_t(t)].piece_count(), tscore);
                traj += tb;
                if (tscore >= -50 || t == 0) break;
                --t;
                tscore = value(before[std::size_t(t)], nullptr).second;
                ++scanned;
            }
            const std::size_t tt = std::size_t(t);
            const int tafter = -value(after[tt], nullptr).second;
            ++scanned;
            char tbuf[256];
            std::snprintf(tbuf, sizeof tbuf, "\t%zu\t%d\t%d\t%s\t%d\t%d", ply[tt], before[tt].piece_count(), tscore,
                          g.moves[ply[tt]].c_str(), tafter, ingame(ply[tt]));
            char buf[256];
            std::snprintf(buf, sizeof buf, "\t%zu\t%zu\t%d\t%d\t%s\t%d\t%d\t%s", ply[b],
                          (ply[b] + (first_mover == WHITE ? 1 : 0)) / 2 + 1,
                          before[b].piece_count(), after[b].piece_count(), best.c_str(), bscore, -opp_score[b],
                          bproven ? "before-proven" : "before-open");
            std::string comment = g.comments[ply[b]];
            for (char& c : comment)
                if (c == '\t' || c == '\n') c = ' ';
            rows[j] = row + buf + "\t" + g.moves[ply[b]] + "\t" + before[b].fen() + "\t" + after[b].fen() + "\t" +
                      std::to_string(scanned) + "\t" + comment + tbuf + "\t" + before[tt].fen() + "\t" +
                      after[tt].fen() + "\t" + traj;
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    std::ofstream o(out);
    o << "file\tblack\twhite\tround\topening\tregicide\ttermination\tply\tmove_no\tpieces_before\tpieces_after\tdeep_best\t"
         "deep_score_before\tdeep_score_after\tbefore_status\tplayed\tfen_before\tfen_after\tsearched\tingame_comment\t"
         "turn_ply\tturn_pieces\tturn_deep_before\tturn_played\tturn_deep_after\tturn_ingame_score\tturn_fen_before\t"
         "turn_fen_after\ttrajectory(ply:pieces:deep score)\n";
    for (const auto& r : rows) o << r << '\n';
    std::printf("losswalk: %zu lost games -> %s\n", jobs.size(), out.c_str());
    return 0;
}

int split(const std::string& in, const std::string& out, int lo, int hi, bool proven_only, long from) {
    Reader r(in);
    std::FILE* o = std::fopen(out.c_str(), "wb");
    if (!r.f || !o) return std::cerr << "cannot open files\n", 1;
    // Game-start flags survive filtering: once the input has shown a flag, the first RETAINED record of each game
    // (and the first record after --from) carries GAME_START, so game-wise tools (relabel, census) do not merge games.
    // Legacy files without flags stay without flags (their games are split by the heuristic).
    DataRecord d;
    long i = 0, n = 0;
    bool seen_flag = false, pending = false;
    while (r.next(d)) {
        if (d.game_start()) seen_flag = pending = true;
        if (i++ < from) continue;
        if (i - 1 == from && seen_flag) pending = true;  // the cut may fall inside a game
        if (d.pieces() < lo || d.pieces() > hi || (proven_only && !d.proven())) continue;
        if (pending) d.stm |= DataRecord::GAME_START;
        pending = false;
        std::fwrite(&d, 16, 1, o);
        ++n;
    }
    std::fclose(o);
    std::printf("split %s: %ld records\n", out.c_str(), n);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) return std::cerr << "usage: truth census|relabel|pilot|split ...\n", 2;
    const std::string cmd = argv[1];
    std::string egdb_dir, in, out = "pilot.csv", opponents;
    std::vector<std::string> files;
    std::vector<std::uint64_t> budgets{10000, 50000, 200000};
    long n = 200000, from = 0;
    int lo = 9, hi = 14, threads = 8;
    bool proven = false;
    std::uint64_t seed = 1;
    for (int i = 2; i < argc; ++i) {
        const std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--egdb") egdb_dir = next();
        else if (k == "--in") in = next();
        else if (k == "--out") out = next();
        else if (k == "--n") n = std::stol(next());
        else if (k == "--from") from = std::stol(next());
        else if (k == "--min") lo = std::stoi(next());
        else if (k == "--max") hi = std::stoi(next());
        else if (k == "--threads") threads = std::stoi(next());
        else if (k == "--seed") seed = std::stoull(next());
        else if (k == "--proven") proven = true;
        else if (k == "--opponents") opponents = lower(next());
        else if (k == "--nodes") {
            budgets.clear();
            std::string s = next();
            for (std::size_t p = 0; p < s.size();) {
                const std::size_t c = std::min(s.find(',', p), s.size());
                budgets.push_back(std::stoull(s.substr(p, c - p)));
                p = c + 1;
            }
        } else if (!k.empty() && k[0] == '-') return std::cerr << "unknown option " << k << '\n', 2;
        else files.push_back(k);
    }
    if (cmd == "pdn") {  // FILE...: the PDN reader's view, one "game ply move {comment}" line per move (tests/truth_pdn_test.cmake)
        int gi = 0;
        for (const auto& file : files)
            for (const auto& g : read_pdn(file)) {
                ++gi;
                for (std::size_t p = 0; p < g.moves.size(); ++p)
                    std::cout << gi << ' ' << p + 1 << ' ' << g.moves[p] << " {" << g.comments[p] << "}\n";
            }
        return 0;
    }
    if (cmd == "split") return split(in, out, lo, hi, proven, from);
    if (cmd == "seeds") return seeds(files, lo, hi, opponents, out);
    if (cmd == "fens") {  // the input is checked before the database loads (tested by tests/truth_fens_test.cmake)
        std::FILE* f = std::fopen(in.c_str(), "rb");
        if (!f) return std::cerr << "cannot open " << in << '\n', 1;
        std::fseek(f, 0, SEEK_END);
        const long bytes = std::ftell(f);
        std::fclose(f);
        if (bytes / 16 <= 0) return std::cerr << in << " holds no complete record\n", 1;
    }
    if (cmd == "pilot") {  // likewise: the inputs are checked before the database loads (same ctest)
        long total = 0;
        for (const auto& file : files) {
            std::FILE* f = std::fopen(file.c_str(), "rb");
            if (!f) return std::cerr << "cannot open " << file << '\n', 1;
            std::fseek(f, 0, SEEK_END);
            total += std::ftell(f) / 16;
            std::fclose(f);
        }
        if (total <= 0) return std::cerr << "the input holds no complete record\n", 1;
    }
    if (egdb_dir.empty() || !egdb::init(egdb_dir, 4096)) return std::cerr << "cannot load egdb (--egdb DIR)\n", 1;
    if (cmd == "census") return census(files);
    if (cmd == "relabel") return relabel(in, out);
    if (cmd == "pilot") return pilot(files, n, lo, hi, budgets, threads, seed, out);
    if (cmd == "losswalk") return losswalk(files, opponents.empty() ? "regicide" : opponents, budgets[0], threads, out);
    if (cmd == "fens") {  // --in F --min A --max B --n K --seed S: random records as "FEN database-value(stm)"
        std::FILE* f = std::fopen(in.c_str(), "rb");
        if (!f) return std::cerr << "cannot open " << in << '\n', 1;
        std::fseek(f, 0, SEEK_END);
        const long count = std::ftell(f) / 16;
        if (count <= 0) return std::fclose(f), std::cerr << in << " holds no complete record\n", 1;
        std::mt19937_64 rng(seed);
        for (long got = 0, tries = 0; got < n && tries < 1000 * n; ++tries) {
            DataRecord r;
            std::fseek(f, long(rng() % std::uint64_t(count)) * 16, SEEK_SET);
            if (std::fread(&r, 16, 1, f) != 1 || r.pieces() < lo || r.pieces() > hi) continue;
            const egdb::Wld w = egdb::probe(r.black, r.white, r.kings, r.color());
            std::printf("%s %s\n", r.position().fen().c_str(),
                        w == egdb::Wld::Win ? "win" : w == egdb::Wld::Loss ? "loss" : w == egdb::Wld::Draw ? "draw" : "unknown");
            ++got;
        }
        std::fclose(f);
        return 0;
    }
    if (cmd == "probe") {  // database value (side to move) of each FEN
        for (const auto& fen : files) {
            Position p;
            if (!p.set_fen(fen)) {
                std::printf("%s bad-fen\n", fen.c_str());
                continue;
            }
            const egdb::Wld w = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
            std::printf("%s %s\n", fen.c_str(), w == egdb::Wld::Win ? "win" : w == egdb::Wld::Loss ? "loss" : w == egdb::Wld::Draw ? "draw" : "unknown");
        }
        return 0;
    }
    std::cerr << "unknown command " << cmd << '\n';
    return 2;
}
