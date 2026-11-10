#!/usr/bin/env python3
"""WP3: add --hash, --egdb/--egdb-cache and --lines to tools/bookgen.cpp (string patch, fails loudly)."""
import sys

path = sys.argv[1]
src = open(path).read()


def rep(old, new, count=1):
    global src
    n = src.count(old)
    if n != count:
        sys.exit(f"patch failed: expected {count} occurrence(s), found {n}:\n{old}")
    src = src.replace(old, new)


rep("""//           [--avoid avoid.bin] [--roots roots.txt]
""", """//           [--avoid avoid.bin] [--roots roots.txt] [--hash MB] [--egdb DIR] [--egdb-cache MB] [--lines lines.txt]
""")
rep("""// also embeds the avoid list. Avoid entries of a --resume book are kept as well.
""", """// also embeds the avoid list. Avoid entries of a --resume book are kept as well.
//
// --hash <MB>: transposition table per search thread (default 64, as in all earlier runs).
// --egdb <dir>: endgame database for the searches (initialised as in lossprobe, cache --egdb-cache MB, default 4096;
// the REGICIDE_EGDB environment variable is not consulted, so old command lines reproduce).
// --lines <file>: forced analysis. Each line is a move sequence from the start position, or "FEN | moves" (the FEN
// is then treated as ply 3, like a --roots entry). Every position on every line (the start of the line, the position
// after each move, and the final one) is analysed after the roots, whatever its priority or ply (also beyond
// --max-ply), and kept on --resume. Lines do not seed the best-first frontier: expansions still grow from the roots.
""")
rep("""    std::string roots;  // optional file of root FENs
};""", """    std::string roots;  // optional file of root FENs
    std::size_t hash_mb = 64;
    std::string egdb;          // endgame database directory (empty: none)
    std::size_t egdb_cache_mb = 4096;
    std::string lines;         // optional file of lines whose positions are always analysed
};

int egdb_pieces = 0;""")
# both thread pools
rep("""                Engine engine;
                engine.set_hash_mb(64);
                engine.set_evaluator(ev);""", """                Engine engine;
                engine.set_hash_mb(cfg.hash_mb);
                engine.set_evaluator(ev);
                engine.set_egdb_pieces(egdb_pieces);""", count=2)
rep("""#include "book.hpp"
""", """#include "book.hpp"
#include "egdb/egdb.hpp"
""")
rep("""        else if (k == "--roots") cfg.roots = next();
""", """        else if (k == "--roots") cfg.roots = next();
        else if (k == "--hash") cfg.hash_mb = std::stoull(next());
        else if (k == "--egdb") cfg.egdb = next();
        else if (k == "--egdb-cache") cfg.egdb_cache_mb = std::stoull(next());
        else if (k == "--lines") cfg.lines = next();
""")
rep("""    avoid_list.finalize();
    if (avoid_list.avoid_size()) std::printf("avoid list: %zu entries\\n", avoid_list.avoid_size());
""", """    if (!cfg.egdb.empty()) {
        if (!egdb::init(cfg.egdb, cfg.egdb_cache_mb)) {
            std::cerr << "no endgame database in " << cfg.egdb << '\\n';
            return 1;
        }
        egdb_pieces = egdb::max_pieces();
    }
    std::printf("hash %zu MB per thread, egdb %d pieces\\n", cfg.hash_mb, egdb_pieces);

    avoid_list.finalize();
    if (avoid_list.avoid_size()) std::printf("avoid list: %zu entries\\n", avoid_list.avoid_size());
""")
# parse lines after the roots are read (before resume)
rep("""    // Resume: rebuild analysed nodes from an existing book by walking from the roots.
""", """    // Forced lines: every position on them, with its ply.
    std::vector<std::pair<Position, int>> line_positions;
    if (!cfg.lines.empty()) {
        std::FILE* in = std::fopen(cfg.lines.c_str(), "r");
        if (!in) {
            std::cerr << "cannot open " << cfg.lines << '\\n';
            return 1;
        }
        std::set<Key> seen;
        char buf[8192];
        int lineno = 0, nlines = 0;
        while (std::fgets(buf, sizeof buf, in)) {
            ++lineno;
            std::string line(buf);
            if (auto h = line.find('#'); h != std::string::npos) line.erase(h);
            std::string fen, moves = line;
            Position p;
            int ply = 0;
            if (auto bar = line.find('|'); bar != std::string::npos) {
                fen = line.substr(0, bar);
                moves = line.substr(bar + 1);
                while (!fen.empty() && std::isspace((unsigned char)fen.back())) fen.pop_back();
                while (!fen.empty() && std::isspace((unsigned char)fen.front())) fen.erase(0, 1);
                if (!p.set_fen(fen)) {
                    std::cerr << cfg.lines << ":" << lineno << ": bad FEN " << fen << '\\n';
                    return 1;
                }
                ply = 3;
            }
            std::vector<std::string> toks;
            {
                std::string t;
                for (char c : moves + " ") {
                    if (std::isspace((unsigned char)c)) {
                        if (!t.empty()) toks.push_back(t);
                        t.clear();
                    } else {
                        t += c;
                    }
                }
            }
            if (fen.empty() && toks.empty()) continue;
            ++nlines;
            auto push = [&](const Position& q, int qply) {
                if (seen.insert(q.key()).second) line_positions.push_back({q, qply});
            };
            push(p, ply);
            for (const std::string& t : toks) {
                if (t.back() == '.' || t == "*" || t == "1-0" || t == "0-1" || t == "1/2-1/2") continue;  // move numbers, results
                Move m;
                if (!parse_move(p, t, m)) {
                    std::cerr << cfg.lines << ":" << lineno << ": illegal move " << t << " in " << p.fen() << '\\n';
                    return 1;
                }
                p.make_move(m);
                push(p, ++ply);
            }
        }
        std::fclose(in);
        std::printf("lines from %s: %d lines, %zu distinct positions\\n", cfg.lines.c_str(), nlines,
                    line_positions.size());
    }

    // Resume: rebuild analysed nodes from an existing book by walking from the roots (and the forced lines).
""")
rep("""        for (const Position& r : root_list) stack.push_back({r, r == Position() ? 0 : 3});
""", """        for (const Position& r : root_list) stack.push_back({r, r == Position() ? 0 : 3});
        for (const auto& lp : line_positions) stack.push_back(lp);
""")
# analyse the line positions after the roots
rep("""        backup_all();
        std::printf("roots analysed: %zu positions\\n", tree.size());
        std::fflush(stdout);
    }
""", """        backup_all();
        std::printf("roots analysed: %zu positions\\n", tree.size());
        std::fflush(stdout);
    }

    // Then every forced-line position not analysed yet.
    if (!line_positions.empty()) {
        std::vector<std::pair<Position, int>> todo;
        for (const auto& lp : line_positions)
            if (!tree.count(lp.first.key())) todo.push_back(lp);
        std::printf("line positions to analyse: %zu of %zu\\n", todo.size(), line_positions.size());
        std::fflush(stdout);
        std::atomic<std::size_t> next{0}, finished{0};
        std::vector<std::thread> pool;
        for (int t = 0; t < cfg.threads; ++t)
            pool.emplace_back([&] {
                Engine engine;
                engine.set_hash_mb(cfg.hash_mb);
                engine.set_evaluator(ev);
                engine.set_egdb_pieces(egdb_pieces);
                for (std::size_t i; (i = next++) < todo.size();) {
                    Node n = analyse(engine, todo[i].first, todo[i].second, cfg);
                    std::lock_guard lock(tree_mu);
                    tree.emplace(n.pos.key(), std::move(n));
                    std::printf("line position %zu/%zu analysed\\n", ++finished, todo.size());
                    std::fflush(stdout);
                }
            });
        for (auto& th : pool) th.join();
        backup_all();
        save(cfg);
        std::printf("lines analysed: %zu positions\\n", tree.size());
        std::fflush(stdout);
    }
""")
rep("""#include <algorithm>
#include <atomic>
""", """#include <algorithm>
#include <atomic>
#include <cctype>
""")
open(path, "w").write(src)
print("patched", path)
