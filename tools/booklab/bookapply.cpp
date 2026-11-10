// booklab_apply: builds the book binary from a base book and a reviewable decisions file (WP4).
//
//   booklab_apply --book base.bin --decisions decisions.txt --out book.bin [--report report.txt]
//
// decisions.txt: one decision per line, "FEN | move | force|avoid | evidence"; '#' starts a comment line.
//   force: in that position the book plays exactly this move (all other book moves of the position are removed; the
//          move keeps its analysed score and depth if the base book had it). Works at any ply, also beyond bookgen's
//          --max-ply, since the engine probes the book by position key.
//   avoid: the move is added to the avoid list (never played at the root unless every legal move is avoided; the
//          book probe skips it).
//   force-unavoid: as force, for a move on the base book's avoid list: that one avoid entry (position and move) is
//          removed and the removal is reported (WP13: a defence two sources agree on, against a lossprobe entry).
// Every line must carry evidence (non-empty fourth field). Conflicts (force and avoid of the same move, two forces in
// one position, forcing a move on the base avoid list without force-unavoid) are errors. After writing, the output is reloaded and every
// decision is verified with Book::probe / Book::avoids.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "movetext.hpp"

using namespace regicide;

namespace {

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

bool read_raw(const std::string& path, std::vector<Book::Entry>& entries, std::vector<Book::Entry>& avoid) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<char> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (d.size() < 12 || std::memcmp(d.data(), "RGBK", 4) != 0) return false;
    std::uint32_t version, count, avoid_count = 0;
    std::memcpy(&version, d.data() + 4, 4);
    std::memcpy(&count, d.data() + 8, 4);
    if (version == 2) {
        if (d.size() < 16) return false;  // the avoid count is part of the version-2 header (as in Book::load)
        std::memcpy(&avoid_count, d.data() + 12, 4);
    } else if (version != 1) {
        return false;
    }
    const std::size_t h = version == 2 ? 16 : 12;
    if (d.size() != h + (std::size_t(count) + avoid_count) * sizeof(Book::Entry)) return false;
    entries.resize(count);
    avoid.resize(avoid_count);
    std::memcpy(entries.data(), d.data() + h, count * sizeof(Book::Entry));
    std::memcpy(avoid.data(), d.data() + h + count * sizeof(Book::Entry), avoid_count * sizeof(Book::Entry));
    return true;
}

bool same_move(const Book::Entry& e, const Book::Entry& m) {
    return e.key == m.key && e.from == m.from && e.to == m.to && e.disc == m.disc;
}

struct Decision {
    int line;
    Position pos;
    Move move;
    bool force;
    bool unavoid = false;  // force-unavoid: the base avoid entry of this exact (position, move) is removed
};

}  // namespace

int main(int argc, char** argv) {
    std::string base, decisions, out, report;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--book") base = next();
        else if (k == "--decisions") decisions = next();
        else if (k == "--out") out = next();
        else if (k == "--report") report = next();
        else {
            std::cerr << "usage: booklab_apply --book base.bin --decisions decisions.txt --out book.bin [--report r.txt]\n";
            return 2;
        }
    }
    if (base.empty() || decisions.empty() || out.empty()) {
        std::cerr << "--book, --decisions and --out are required\n";
        return 2;
    }
    std::vector<Book::Entry> entries, avoid;
    if (!read_raw(base, entries, avoid)) {
        std::cerr << "cannot read book " << base << '\n';
        return 1;
    }
    std::ostringstream rep;
    rep << "base " << base << ": " << entries.size() << " entries, " << avoid.size() << " avoid entries\n";

    std::ifstream df(decisions);
    if (!df) {
        std::cerr << "cannot open " << decisions << '\n';
        return 1;
    }
    std::vector<Decision> ds;
    std::string line;
    int lineno = 0, errors = 0;
    while (std::getline(df, line)) {
        ++lineno;
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        std::vector<std::string> f;
        std::size_t a = 0;
        for (int k = 0; k < 3; ++k) {
            const auto b = t.find('|', a);
            if (b == std::string::npos) break;
            f.push_back(trim(t.substr(a, b - a)));
            a = b + 1;
        }
        f.push_back(trim(t.substr(a)));
        auto fail = [&](const std::string& why) {
            std::cerr << decisions << ":" << lineno << ": " << why << '\n';
            ++errors;
        };
        if (f.size() != 4) { fail("expected 'FEN | move | force|avoid | evidence'"); continue; }
        if (f[3].empty()) { fail("missing evidence"); continue; }
        Decision d{lineno, Position(), Move(), false};
        if (!d.pos.set_fen(f[0])) { fail("bad FEN " + f[0]); continue; }
        if (!parse_move(d.pos, f[1], d.move)) { fail("illegal move " + f[1] + " in " + f[0]); continue; }
        if (const int k = booklab::matching_moves(d.pos, f[1]); k != 1) {
            fail("move text " + f[1] + " matches " + std::to_string(k) + " legal moves in " + f[0] +
                 "; write the full capture path");
            continue;
        }
        if (f[2] == "force") d.force = true;
        else if (f[2] == "force-unavoid") d.force = d.unavoid = true;
        else if (f[2] != "avoid") { fail("third field must be force, force-unavoid or avoid, got " + f[2]); continue; }
        ds.push_back(d);
    }
    // Conflicts.
    for (std::size_t i = 0; i < ds.size(); ++i)
        for (std::size_t j = i + 1; j < ds.size(); ++j) {
            if (ds[i].pos.key() != ds[j].pos.key()) continue;
            const bool same = ds[i].move == ds[j].move;
            if (ds[i].force && ds[j].force && !same) {
                std::cerr << "lines " << ds[i].line << " and " << ds[j].line << ": two different forced moves\n";
                ++errors;
            }
            if (ds[i].force != ds[j].force && same) {
                std::cerr << "lines " << ds[i].line << " and " << ds[j].line << ": force and avoid of the same move\n";
                ++errors;
            }
        }
    for (const Decision& d : ds) {
        if (!d.force) continue;
        const Book::Entry me = Book::make_entry(d.pos, d.move, 0, 0);
        bool on_list = false;
        for (const Book::Entry& e : avoid)
            if (same_move(e, me)) on_list = true;
        if (on_list && !d.unavoid) {
            std::cerr << "line " << d.line << ": forced move " << booklab::move_text(d.pos, d.move)
                      << " is on the base avoid list (force-unavoid overrides it, with evidence)\n";
            ++errors;
        } else if (!on_list && d.unavoid) {
            std::cerr << "line " << d.line << ": force-unavoid, but " << booklab::move_text(d.pos, d.move)
                      << " is not on the base avoid list\n";
            ++errors;
        } else if (on_list) {
            const auto before = avoid.size();
            std::erase_if(avoid, [&](const Book::Entry& e) { return same_move(e, me); });
            rep << "line " << d.line << ": force-unavoid " << booklab::move_text(d.pos, d.move) << " in "
                << d.pos.fen() << ": removed " << before - avoid.size() << " base avoid entry\n";
        }
    }
    if (errors) {
        std::cerr << errors << " error(s); nothing written\n";
        return 1;
    }

    int forced = 0, avoided = 0;
    for (const Decision& d : ds) {
        const Book::Entry me = Book::make_entry(d.pos, d.move, 0, 0);
        std::ostringstream before;
        int best = -32768;
        Book::Entry keep = me;
        bool had = false;
        for (const Book::Entry& e : entries)
            if (e.key == me.key) {
                if (auto m = Book::to_move(d.pos, e))
                    before << ' ' << booklab::move_text(d.pos, *m) << ':' << e.score << "/d" << int(e.depth);
                best = std::max(best, int(e.score));
                if (same_move(e, me)) keep = e, had = true;
            }
        if (d.force) {
            if (!had) keep.score = std::int16_t(best == -32768 ? 0 : best);
            std::erase_if(entries, [&](const Book::Entry& e) { return e.key == me.key; });
            entries.push_back(keep);
            ++forced;
        } else {
            avoid.push_back(me);
            ++avoided;
        }
        rep << "line " << d.line << ": " << (d.force ? "force " : "avoid ") << booklab::move_text(d.pos, d.move) << " in "
            << d.pos.fen() << "; base book moves:" << (before.str().empty() ? " (none)" : before.str()) << '\n';
    }
    Book book;
    for (const Book::Entry& e : entries) book.add(e);
    for (const Book::Entry& e : avoid) book.add_avoid(e);
    book.finalize();
    if (!book.save(out)) {
        std::cerr << "cannot write " << out << '\n';
        return 1;
    }
    Book check;
    if (!check.load(out)) {
        std::cerr << "cannot reload " << out << '\n';
        return 1;
    }
    int bad = 0;
    for (const Decision& d : ds) {
        if (d.force) {
            auto m = check.probe(d.pos);
            if (!m || !(*m == d.move)) {
                std::cerr << "verify failed, line " << d.line << ": book does not play the forced move\n";
                ++bad;
            }
        } else if (!check.avoids(d.pos, d.move)) {
            std::cerr << "verify failed, line " << d.line << ": move not avoided\n";
            ++bad;
        }
    }
    rep << "decisions: " << forced << " force, " << avoided << " avoid; output " << out << ": " << check.size()
        << " entries, " << check.avoid_size() << " avoid entries; verify " << (bad ? "FAILED" : "ok") << '\n';
    std::cout << rep.str();
    if (!report.empty()) std::ofstream(report) << rep.str();
    return bad ? 1 : 0;
}
