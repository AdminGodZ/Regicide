#include "checkers.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdlib>

namespace cbt {

namespace {

// Neighbour tables. Direction d: 0 = (+1,+1), 1 = (-1,+1), 2 = (+1,-1), 3 = (-1,-1) in Board8x8
// coordinates. +y is the direction Black men move.
struct Tables {
    std::array<std::array<std::int8_t, 4>, 32> step{};
    std::array<std::array<std::int8_t, 4>, 32> jump{};
    Tables() {
        static constexpr int dx[4] = {1, -1, 1, -1};
        static constexpr int dy[4] = {1, 1, -1, -1};
        for (int sq = 0; sq < 32; ++sq) {
            int x, y;
            square_xy(sq, x, y);
            for (int d = 0; d < 4; ++d) {
                step[sq][d] = std::int8_t(xy_square(x + dx[d], y + dy[d]));
                jump[sq][d] = std::int8_t(xy_square(x + 2 * dx[d], y + 2 * dy[d]));
            }
        }
    }
};
const Tables& tables() {
    static const Tables t;
    return t;
}

constexpr std::uint32_t BLACK_KING_ROW = 0xF0000000u;  // squares 29..32
constexpr std::uint32_t WHITE_KING_ROW = 0x0000000Fu;  // squares 1..4

std::uint32_t king_row(Color c) { return c == Color::Black ? BLACK_KING_ROW : WHITE_KING_ROW; }

// Directions a piece may move in: men only forward.
void directions(Color c, bool king, int dirs[4], int& n) {
    n = 0;
    if (king || c == Color::Black) { dirs[n++] = 0; dirs[n++] = 1; }
    if (king || c == Color::White) { dirs[n++] = 2; dirs[n++] = 3; }
}

void add_unique(std::vector<Move>& out, const Move& m) {
    for (const Move& o : out)
        if (o.same_effect(m)) return;
    out.push_back(m);
}

// Captured pieces are removed as they are jumped (CheckerBoard semantics). In English checkers
// a landing square can never be a previously jumped square (they differ in coordinate parity),
// so this is equivalent to removing them at the end of the move.
void extend_jumps(std::uint32_t empty, std::uint32_t opp, int sq, Color c, bool king, Move& cur,
                  std::vector<Move>& out) {
    const Tables& t = tables();
    int dirs[4], nd;
    directions(c, king, dirs, nd);
    bool extended = false;
    for (int i = 0; i < nd; ++i) {
        int d = dirs[i];
        int over = t.step[sq][d], land = t.jump[sq][d];
        if (land < 0 || !(opp & bit(over)) || !(empty & bit(land))) continue;
        extended = true;
        Move next = cur;
        next.captures |= bit(over);
        next.path[next.njumps++] = std::int8_t(land);
        next.to = std::int8_t(land);
        bool crowns = !king && (bit(land) & king_row(c));
        if (crowns) {
            add_unique(out, next);  // crowning ends the move
            continue;
        }
        extend_jumps((empty | bit(sq) | bit(over)) & ~bit(land), opp & ~bit(over), land, c, king, next, out);
    }
    if (!extended && cur.njumps > 0) add_unique(out, cur);
}

}  // namespace

const char* color_name(Color c) { return c == Color::Black ? "black" : "white"; }

void square_xy(int sq, int& x, int& y) {
    y = sq / 4;
    x = 2 * (3 - sq % 4) + (y & 1);
}

int xy_square(int x, int y) {
    if (x < 0 || x > 7 || y < 0 || y > 7 || ((x + y) & 1) != 0) return -1;
    return 4 * y + 3 - x / 2;
}

Board Board::start() {
    Board b;
    b.black = 0x00000FFFu;
    b.white = 0xFFF00000u;
    return b;
}

std::vector<Move> Board::legal_moves() const {
    const Tables& t = tables();
    std::vector<Move> out;
    const Color c = stm;
    const std::uint32_t own = pieces(c), opp = pieces(opposite(c)), empty = ~occupied();

    for (std::uint32_t bb = own; bb; bb &= bb - 1) {
        int sq = std::countr_zero(bb);
        Move m;
        m.from = m.to = std::int8_t(sq);
        extend_jumps(empty | bit(sq), opp, sq, c, (kings & bit(sq)) != 0, m, out);
    }
    if (!out.empty()) return out;

    for (std::uint32_t bb = own; bb; bb &= bb - 1) {
        int sq = std::countr_zero(bb);
        int dirs[4], nd;
        directions(c, (kings & bit(sq)) != 0, dirs, nd);
        for (int i = 0; i < nd; ++i) {
            int to = t.step[sq][dirs[i]];
            if (to < 0 || !(empty & bit(to))) continue;
            Move m;
            m.from = std::int8_t(sq);
            m.to = std::int8_t(to);
            out.push_back(m);
        }
    }
    return out;
}

bool Board::promotes(const Move& m) const {
    return !(kings & bit(m.from)) && (bit(m.to) & king_row(stm));
}

bool Board::is_conversion(const Move& m) const {
    return m.is_capture() || !(kings & bit(m.from));
}

Board Board::after(const Move& m) const {
    Board b = *this;
    std::uint32_t& own = (stm == Color::Black) ? b.black : b.white;
    std::uint32_t& opp = (stm == Color::Black) ? b.white : b.black;
    const bool was_king = (kings & bit(m.from)) != 0;
    own &= ~bit(m.from);
    b.kings &= ~bit(m.from);
    opp &= ~m.captures;
    b.kings &= ~m.captures;
    own |= bit(m.to);
    if (was_king || (bit(m.to) & king_row(stm))) b.kings |= bit(m.to);
    b.stm = opposite(stm);
    return b;
}

std::string Board::fen() const {
    std::string s(1, stm == Color::Black ? 'B' : 'W');
    for (Color c : {Color::White, Color::Black}) {
        s += c == Color::White ? ":W" : ":B";
        bool first = true;
        for (std::uint32_t bb = pieces(c); bb; bb &= bb - 1) {
            int sq = std::countr_zero(bb);
            if (!first) s += ',';
            first = false;
            if (kings & bit(sq)) s += 'K';
            s += std::to_string(sq + 1);
        }
    }
    return s;
}

std::optional<Board> Board::from_fen(std::string_view fen, std::string* err) {
    auto fail = [&](const std::string& why) -> std::optional<Board> {
        if (err) *err = "bad FEN '" + std::string(fen) + "': " + why;
        return std::nullopt;
    };
    std::string s;
    for (char ch : fen)
        if (!std::isspace(static_cast<unsigned char>(ch)) && ch != '"') s += ch;
    while (!s.empty() && s.back() == '.') s.pop_back();
    auto colon = s.find(':');
    if (colon == std::string::npos || colon == 0) return fail("missing ':'");
    Board b;
    char side = char(std::toupper(static_cast<unsigned char>(s[colon - 1])));
    if (side == 'B') b.stm = Color::Black;
    else if (side == 'W') b.stm = Color::White;
    else return fail("side to move must be B or W");

    std::size_t pos = colon + 1;
    while (pos < s.size()) {
        std::size_t end = s.find(':', pos);
        if (end == std::string::npos) end = s.size();
        std::string_view sec(s.data() + pos, end - pos);
        pos = end + 1;
        if (sec.empty()) continue;
        char who = char(std::toupper(static_cast<unsigned char>(sec[0])));
        if (who != 'B' && who != 'W') return fail("section must start with B or W");
        std::uint32_t& set = who == 'B' ? b.black : b.white;
        std::size_t i = 1;
        while (i < sec.size()) {
            if (sec[i] == ',') { ++i; continue; }
            bool king = false;
            if (std::toupper(static_cast<unsigned char>(sec[i])) == 'K') { king = true; ++i; }
            auto num = [&](int& v) {
                std::size_t st = i;
                while (i < sec.size() && std::isdigit(static_cast<unsigned char>(sec[i]))) ++i;
                if (st == i) return false;
                v = std::atoi(std::string(sec.substr(st, i - st)).c_str());
                return v >= 1 && v <= 32;
            };
            int a, z;
            if (!num(a)) return fail("bad square number");
            z = a;
            if (i < sec.size() && sec[i] == '-') {
                ++i;
                if (!num(z) || z < a) return fail("bad square range");
            }
            for (int n = a; n <= z; ++n) {
                set |= bit(n - 1);
                if (king) b.kings |= bit(n - 1);
            }
        }
    }
    if (b.black & b.white) return fail("square occupied twice");
    return b;
}

void Board::to_board8(int b[8][8]) const {
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y) b[x][y] = 0;
    for (int sq = 0; sq < 32; ++sq) {
        int x, y;
        square_xy(sq, x, y);
        int v = 0;
        if (black & bit(sq)) v = 2;       // CB_BLACK
        else if (white & bit(sq)) v = 1;  // CB_WHITE
        if (v) v |= (kings & bit(sq)) ? 8 : 4;
        b[x][y] = v;
    }
}

Board Board::from_board8(const int b[8][8], Color stm) {
    Board r;
    r.stm = stm;
    for (int sq = 0; sq < 32; ++sq) {
        int x, y;
        square_xy(sq, x, y);
        int v = b[x][y];
        if (v & 2) r.black |= bit(sq);
        else if (v & 1) r.white |= bit(sq);
        if ((v & 3) && (v & 8)) r.kings |= bit(sq);
    }
    return r;
}

std::string Board::pretty() const {
    std::string s;
    for (int y = 0; y < 8; ++y) {  // y = 0 holds squares 1..4 (Black's back rank)
        for (int x = 7; x >= 0; --x) {
            int sq = xy_square(x, y);
            char ch = ' ';
            if (sq >= 0) {
                ch = '.';
                if (black & bit(sq)) ch = (kings & bit(sq)) ? 'B' : 'b';
                if (white & bit(sq)) ch = (kings & bit(sq)) ? 'W' : 'w';
            }
            s += ch;
            s += ' ';
        }
        s += '\n';
    }
    s += std::string(color_name(stm)) + " to move\n";
    return s;
}

std::string move_to_pdn(const Move& m) {
    std::string s = std::to_string(m.from + 1);
    if (!m.is_capture()) return s + "-" + std::to_string(m.to + 1);
    for (int i = 0; i < m.njumps; ++i) s += "x" + std::to_string(m.path[i] + 1);
    return s;
}

std::optional<Move> parse_move(const Board& b, std::string_view text, std::string* err, std::uint32_t captures) {
    std::vector<int> sq;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            if (text[i] != '-' && text[i] != 'x' && text[i] != 'X' && text[i] != ':') {
                if (err) *err = "bad move text '" + std::string(text) + "'";
                return std::nullopt;
            }
            ++i;
            continue;
        }
        int v = 0;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) v = v * 10 + (text[i++] - '0');
        if (v < 1 || v > 32) {
            if (err) *err = "square out of range in '" + std::string(text) + "'";
            return std::nullopt;
        }
        sq.push_back(v - 1);
    }
    if (sq.size() < 2) {
        if (err) *err = "bad move text '" + std::string(text) + "'";
        return std::nullopt;
    }
    // The pieces a written path jumps: 0 unless every step is a jump and no piece is jumped twice. A path names a
    // move by what it captures, so the routes around a circle that take the same pieces are the same move.
    std::uint32_t jumped = 0;
    bool is_path = true;
    for (std::size_t k = 0; is_path && k + 1 < sq.size(); ++k) {
        int over = -1;
        for (int d = 0; d < 4; ++d)
            if (tables().jump[sq[k]][d] == sq[k + 1]) over = tables().step[sq[k]][d];
        is_path = over >= 0 && !(jumped & bit(over));
        if (is_path) jumped |= bit(over);
    }
    if (!is_path) jumped = 0;

    std::vector<Move> cands;
    for (const Move& m : b.legal_moves()) {
        if (m.from != sq.front() || m.to != sq.back()) continue;
        if (sq.size() > 2 && (!m.is_capture() || m.captures != jumped)) continue;
        if (captures && m.captures != captures) continue;
        cands.push_back(m);
    }
    // Two squares can be a single jump written out in full or the ends of a longer capture: the exact reading wins.
    if (cands.size() > 1 && sq.size() == 2 && jumped)
        for (const Move& m : cands)
            if (m.captures == jumped) return m;
    if (cands.size() == 1) return cands.front();
    if (err) {
        if (cands.empty()) *err = "illegal move '" + std::string(text) + "' in " + b.fen();
        else *err = "ambiguous move '" + std::string(text) + "' in " + b.fen();
    }
    return std::nullopt;
}

std::optional<Board> play_moves(Board b, std::string_view moves, std::vector<Move>* played, std::string* err) {
    std::size_t i = 0;
    while (i < moves.size()) {
        while (i < moves.size() && std::isspace(static_cast<unsigned char>(moves[i]))) ++i;
        std::size_t st = i;
        while (i < moves.size() && !std::isspace(static_cast<unsigned char>(moves[i]))) ++i;
        std::string_view tok = moves.substr(st, i - st);
        if (tok.empty()) break;
        // Skip move numbers ("1.", "12...") and results.
        if (tok.back() == '.' || tok == "*" || tok == "1-0" || tok == "0-1" || tok == "1/2-1/2" || tok == "2-0" ||
            tok == "0-2" || tok == "1-1")
            continue;
        auto m = parse_move(b, tok, err);
        if (!m) return std::nullopt;
        if (played) played->push_back(*m);
        b = b.after(*m);
    }
    return b;
}

std::uint64_t perft(const Board& b, int depth) {
    if (depth == 0) return 1;
    auto moves = b.legal_moves();
    if (depth == 1) return moves.size();
    std::uint64_t n = 0;
    for (const Move& m : moves) n += perft(b.after(m), depth - 1);
    return n;
}

}  // namespace cbt
