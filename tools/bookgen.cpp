// Opening book generator: best-first drop-out expansion over a minimax-backed tree.
//
//   bookgen --nodes 3000000 --expansions 20000 --threads 12 --out book.bin [--evalfile w.bin] [--resume book.bin]
//           [--avoid avoid.bin] [--roots roots.txt]
//
// Every analysed position gets a fixed-node search score for each legal move. Leaves (analysed moves whose child
// has not been analysed yet) are expanded in order of priority = ply * drop + "regret", where regret is the score
// the side to move gives up along the path by not playing its best move. Expanded values are backed up by minimax.
// Roots are the start position and every distinct position after 3 plies (covers all 3-move ballot openings), or,
// with --roots, the positions listed in a file (one FEN per line, treated as ply 3), e.g. to deepen only the
// ballots an opponent keeps winning.
//
// --avoid <file>: an avoid list (e.g. from lossprobe). Its moves are never analysed, added or expanded (unless every
// legal move of a position is on it), and the output book carries the list (RGBK version 2), so embedding the book
// also embeds the avoid list. Avoid entries of a --resume book are kept as well.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

struct Config {
    std::uint64_t nodes = 2000000;
    long expansions = 5000;
    int threads = 8;
    int drop = 12;        // regret-equivalent cost per ply of depth
    int max_ply = 40;
    std::string out = "book.bin";
    std::string evalfile;
    std::string roots;  // optional file of root FENs
};

struct MoveInfo {
    Move move;
    int score = 0;  // side to move's view: search score, or backed-up value once the child is analysed
    int depth = 0;
};

struct Node {
    Position pos;
    int ply = 0;
    std::vector<MoveInfo> moves;  // sorted by score after analysis
    int value = 0;
};

// Positions are shared between transpositions; keyed by Zobrist key.
std::unordered_map<Key, Node> tree;
std::mutex tree_mu;
Book avoid_list;  // only its avoid entries are used

// Legal moves minus avoided ones (all of them if every move is avoided, as in the engine).
std::vector<Move> book_moves(const Position& pos) {
    MoveList list;
    generate_moves(pos, list);
    std::vector<Move> out;
    for (const Move& m : list)
        if (!avoid_list.avoids(pos, m)) out.push_back(m);
    if (out.empty()) out.assign(list.begin(), list.end());
    return out;
}

int backed_value(const Node& n) {
    int best = -SCORE_INF;
    for (const MoveInfo& m : n.moves) best = std::max(best, m.score);
    return best;
}

// Recomputes every analysed node's move scores from its analysed children until values stop changing.
void backup_all() {
    for (int iter = 0; iter < 64; ++iter) {
        bool changed = false;
        for (auto& [key, node] : tree) {
            for (MoveInfo& mi : node.moves) {
                Position c = node.pos;
                c.make_move(mi.move);
                auto it = tree.find(c.key());
                if (it != tree.end() && !it->second.moves.empty() && -it->second.value != mi.score) {
                    mi.score = -it->second.value;
                    changed = true;
                }
            }
            const int v = backed_value(node);
            if (!node.moves.empty() && v != node.value) {
                node.value = v;
                changed = true;
            }
        }
        if (!changed) break;
    }
}

struct Leaf {
    long priority;
    Position pos;
    int ply;
    bool operator<(const Leaf& o) const { return priority > o.priority; }  // min-heap
};

// Collects unanalysed children reachable from the roots with their priorities. Regret adds up along a path and a
// position counts with the smallest regret of any path to it, so the nodes are visited in order of regret
// (Dijkstra; a first-come depth-first walk gave transpositions the regret of whichever path reached them first).
std::vector<Leaf> frontier(const std::vector<Position>& roots, const Config& cfg) {
    struct Item {
        long regret;
        Key key;
        bool operator>(const Item& o) const { return regret != o.regret ? regret > o.regret : key > o.key; }
    };
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
    for (const Position& r : roots) queue.push({0, r.key()});
    std::set<Key> done;
    std::map<Key, Leaf> leaves;  // the cheapest occurrence of each leaf
    while (!queue.empty()) {
        const Item it = queue.top();
        queue.pop();
        if (!done.insert(it.key).second) continue;
        const Node& n = tree.at(it.key);
        if (n.moves.empty() || n.ply >= cfg.max_ply) continue;
        for (const MoveInfo& mi : n.moves) {
            const long regret = it.regret + std::max(0, n.value - mi.score);
            Position c = n.pos;
            c.make_move(mi.move);
            auto ct = tree.find(c.key());
            if (ct != tree.end() && !ct->second.moves.empty()) {
                if (!done.contains(c.key())) queue.push({regret, c.key()});
            } else if (!is_decisive(mi.score)) {
                const Leaf leaf{regret + long(cfg.drop) * (n.ply + 1), c, n.ply + 1};
                const auto [at, fresh] = leaves.try_emplace(c.key(), leaf);
                if (!fresh && leaf.priority < at->second.priority) at->second = leaf;
            }
        }
    }
    std::vector<Leaf> out;
    for (const auto& [key, leaf] : leaves) out.push_back(leaf);
    std::stable_sort(out.begin(), out.end(), [](const Leaf& a, const Leaf& b) { return a.priority < b.priority; });
    return out;
}

Node analyse(Engine& engine, const Position& pos, int ply, const Config& cfg) {
    Node n;
    n.pos = pos;
    n.ply = ply;
    Limits limits;
    limits.nodes = cfg.nodes;
    for (const Move& m : book_moves(pos)) {
        Position c = pos;
        c.make_move(m);
        engine.set_position(c);
        SearchResult r = engine.go(limits);
        MoveList cl;
        generate_moves(c, cl);
        const int score = cl.empty() ? SCORE_WIN - 1 : -r.score;
        n.moves.push_back({m, score, r.depth + 1});
    }
    std::sort(n.moves.begin(), n.moves.end(), [](const MoveInfo& a, const MoveInfo& b) { return a.score > b.score; });
    n.value = n.moves.empty() ? -SCORE_WIN : n.moves[0].score;
    return n;
}

std::vector<Position> roots() {
    std::vector<Position> out{Position()};
    std::set<Key> seen;
    auto rec = [&](auto& self, const Position& p, int d) -> void {
        if (d == 0) {
            if (seen.insert(p.key()).second) out.push_back(p);
            return;
        }
        MoveList list;
        generate_moves(p, list);
        for (const Move& m : list) {
            Position c = p;
            c.make_move(m);
            self(self, c, d - 1);
        }
    };
    rec(rec, Position(), 3);
    return out;
}

void save(const Config& cfg) {
    Book book;
    for (const Book::Entry& e : avoid_list.avoid_entries()) book.add_avoid(e);
    for (const auto& [key, node] : tree)
        for (const MoveInfo& mi : node.moves) book.add(Book::make_entry(node.pos, mi.move, mi.score, mi.depth));
    book.finalize();
    book.save(cfg.out);
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    std::string resume;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : "0"; };
        if (k == "--nodes") cfg.nodes = std::stoull(next());
        else if (k == "--expansions") cfg.expansions = std::stol(next());
        else if (k == "--threads") cfg.threads = std::stoi(next());
        else if (k == "--drop") cfg.drop = std::stoi(next());
        else if (k == "--max-ply") cfg.max_ply = std::stoi(next());
        else if (k == "--out") cfg.out = next();
        else if (k == "--evalfile") cfg.evalfile = next();
        else if (k == "--resume") resume = next();
        else if (k == "--roots") cfg.roots = next();
        else if (k == "--avoid") {
            const std::string f = next();
            Book b;
            if (!b.load(f)) {
                std::cerr << "cannot load " << f << '\n';
                return 1;
            }
            for (const Book::Entry& e : b.avoid_entries()) avoid_list.add_avoid(e);
        }
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }
    std::shared_ptr<const Evaluator> ev = Evaluator::builtin();
    if (!cfg.evalfile.empty() && !(ev = Evaluator::from_file(cfg.evalfile))) return 1;

    avoid_list.finalize();
    if (avoid_list.avoid_size()) std::printf("avoid list: %zu entries\n", avoid_list.avoid_size());
    std::vector<Position> root_list;
    if (cfg.roots.empty()) {
        root_list = roots();
    } else {
        std::FILE* in = std::fopen(cfg.roots.c_str(), "r");
        if (!in) {
            std::cerr << "cannot open " << cfg.roots << '\n';
            return 1;
        }
        std::set<Key> seen;
        char line[512];
        while (std::fgets(line, sizeof line, in)) {
            std::string fen(line);
            while (!fen.empty() && (fen.back() == '\n' || fen.back() == '\r' || fen.back() == ' ')) fen.pop_back();
            if (fen.empty() || fen[0] == '#') continue;
            Position p;
            if (!p.set_fen(fen)) {
                std::cerr << "bad FEN in " << cfg.roots << ": " << fen << '\n';
                return 1;
            }
            if (seen.insert(p.key()).second) root_list.push_back(p);
        }
        std::fclose(in);
        std::printf("roots from %s: %zu positions\n", cfg.roots.c_str(), root_list.size());
    }

    // Resume: rebuild analysed nodes from an existing book by walking from the roots.
    if (!resume.empty()) {
        Book old;
        if (!old.load(resume)) {
            std::cerr << "cannot load " << resume << '\n';
            return 1;
        }
        for (const Book::Entry& e : old.avoid_entries()) avoid_list.add_avoid(e);
        avoid_list.finalize();
        std::vector<std::pair<Position, int>> stack;
        for (const Position& r : root_list) stack.push_back({r, r == Position() ? 0 : 3});
        while (!stack.empty()) {
            auto [p, ply] = stack.back();
            stack.pop_back();
            if (tree.count(p.key())) continue;
            std::vector<Book::Entry> es = old.lookup(p);
            if (es.empty()) continue;
            Node n;
            n.pos = p;
            n.ply = ply;
            const std::vector<Move> allowed = book_moves(p);
            for (const Book::Entry& e : es)
                if (auto m = Book::to_move(p, e); m && std::ranges::contains(allowed, *m)) {
                    n.moves.push_back({*m, e.score, e.depth});
                    Position c = p;
                    c.make_move(*m);
                    stack.push_back({c, ply + 1});
                }
            std::sort(n.moves.begin(), n.moves.end(),
                      [](const MoveInfo& a, const MoveInfo& b) { return a.score > b.score; });
            n.value = n.moves.empty() ? 0 : n.moves[0].score;
            tree.emplace(p.key(), std::move(n));
        }
        backup_all();
        std::printf("resumed %zu positions\n", tree.size());
    }

    // Analyse the roots first.
    {
        std::vector<Position> todo;
        for (const Position& r : root_list)
            if (!tree.count(r.key())) todo.push_back(r);
        std::atomic<std::size_t> next{0};
        std::vector<std::thread> pool;
        for (int t = 0; t < cfg.threads; ++t)
            pool.emplace_back([&] {
                Engine engine;
                engine.set_hash_mb(64);
                engine.set_evaluator(ev);
                for (std::size_t i; (i = next++) < todo.size();) {
                    Node n = analyse(engine, todo[i], todo[i] == Position() ? 0 : 3, cfg);
                    std::lock_guard lock(tree_mu);
                    tree.emplace(n.pos.key(), std::move(n));
                }
            });
        for (auto& th : pool) th.join();
        backup_all();
        std::printf("roots analysed: %zu positions\n", tree.size());
        std::fflush(stdout);
    }

    long done = 0;
    while (done < cfg.expansions) {
        // Expand a batch of the most promising leaves in parallel, then back up and recompute the frontier.
        std::vector<Leaf> leaves = frontier(root_list, cfg);
        if (leaves.empty()) break;
        const std::size_t batch = std::min<std::size_t>(leaves.size(), std::size_t(cfg.threads) * 4);
        leaves.resize(batch);
        std::atomic<std::size_t> next{0};
        std::vector<std::thread> pool;
        for (int t = 0; t < cfg.threads; ++t)
            pool.emplace_back([&] {
                Engine engine;
                engine.set_hash_mb(64);
                engine.set_evaluator(ev);
                for (std::size_t i; (i = next++) < leaves.size();) {
                    Node n = analyse(engine, leaves[i].pos, leaves[i].ply, cfg);
                    std::lock_guard lock(tree_mu);
                    tree.emplace(n.pos.key(), std::move(n));
                }
            });
        for (auto& th : pool) th.join();
        done += long(batch);
        backup_all();
        const auto start = tree.find(root_list.front().key());  // the start position, or the first --roots entry
        std::printf("expanded %ld  book positions %zu  next priority %ld  %s value %d\n", done, tree.size(),
                    leaves.back().priority, cfg.roots.empty() ? "start" : "first root",
                    start == tree.end() ? 0 : start->second.value);
        std::fflush(stdout);
        if (done % (cfg.threads * 40) < long(batch)) save(cfg);
    }
    save(cfg);
    std::printf("wrote %s with %zu positions\n", cfg.out.c_str(), tree.size());
    return 0;
}
