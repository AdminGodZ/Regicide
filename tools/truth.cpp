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
//   truth seeds   --min 6 --max 40 --opponents cake,kingsrow,guinn --out seeds.txt PDN...
//                                                    positions after plies min..max of real games (datagen --seeds)
//
// Game boundaries. datagen appends whole games, one at a time, in play order, so a file is a sequence of games. Two
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

// Calls on_game(records) for every game of the file.
template <class F>
long for_each_game(const std::string& path, F on_game) {
    Reader in(path);
    if (!in.f) {
        std::cerr << "cannot open " << path << '\n';
        std::exit(1);
    }
    std::vector<DataRecord> game;
    DataRecord r;
    long games = 0;
    while (in.next(r)) {
        if (!game.empty() && !same_game(game.back(), r)) {
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
        });
        long low = 0;
        for (int p = 0; p <= db; ++p) low += hist[p];
        std::printf("file %s records %ld games %ld avg_len %.1f le%d %ld (%.2f%%)\n", base(file).c_str(), records,
                    games, double(len_sum) / std::max(1L, games), db, low, 100.0 * low / std::max(1L, records));
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
    });
    std::fclose(o);
    std::printf("relabel %s -> %s: records %ld games %ld relabelled %ld\n", base(in).c_str(), base(out).c_str(), records,
                games, changed);
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
    while (long(items.size()) < n) {
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
                scores[i][b] = engine.go(l).score;
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
                if (!is_result(token)) g.moves.push_back(token);
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

int split(const std::string& in, const std::string& out, int lo, int hi, bool proven_only, long from) {
    Reader r(in);
    std::FILE* o = std::fopen(out.c_str(), "wb");
    if (!r.f || !o) return std::cerr << "cannot open files\n", 1;
    DataRecord d;
    long i = 0, n = 0;
    while (r.next(d)) {
        if (i++ < from) continue;
        if (d.pieces() < lo || d.pieces() > hi || (proven_only && !d.proven())) continue;
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
    if (cmd == "split") return split(in, out, lo, hi, proven, from);
    if (cmd == "seeds") return seeds(files, lo, hi, opponents, out);
    if (egdb_dir.empty() || !egdb::init(egdb_dir, 4096)) return std::cerr << "cannot load egdb (--egdb DIR)\n", 1;
    if (cmd == "census") return census(files);
    if (cmd == "relabel") return relabel(in, out);
    if (cmd == "pilot") return pilot(files, n, lo, hi, budgets, threads, seed, out);
    std::cerr << "unknown command " << cmd << '\n';
    return 2;
}
