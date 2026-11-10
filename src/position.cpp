#include "position.hpp"

#include <cctype>
#include <charconv>
#include <sstream>

namespace regicide {

bool Position::set_fen(const std::string& fen) {
    Bitboard pieces[2] = {0, 0};
    Bitboard kings = 0;
    Color stm;

    std::stringstream ss(fen);
    std::string field;
    if (!std::getline(ss, field, ':') || field.empty()) return false;
    switch (std::toupper(static_cast<unsigned char>(field[0]))) {
        case 'B': stm = BLACK; break;
        case 'W': stm = WHITE; break;
        default: return false;
    }

    while (std::getline(ss, field, ':')) {
        if (field.empty()) continue;
        Color c;
        switch (std::toupper(static_cast<unsigned char>(field[0]))) {
            case 'B': c = BLACK; break;
            case 'W': c = WHITE; break;
            default: return false;
        }
        std::stringstream list(field.substr(1));
        std::string tok;
        while (std::getline(list, tok, ',')) {
            const auto space = [](char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; };
            while (!tok.empty() && space(tok.back())) tok.pop_back();
            while (!tok.empty() && space(tok.front())) tok.erase(0, 1);
            if (tok.empty()) continue;
            bool king = std::toupper(static_cast<unsigned char>(tok[0])) == 'K';
            // A square or a range of squares ("21-32"); anything left over in the token is an error.
            const char* p = tok.data() + king;
            const char* const end = tok.data() + tok.size();
            int first = 0, last = 0;
            auto r = std::from_chars(p, end, first);
            if (r.ec != std::errc{}) return false;
            last = first;
            if (r.ptr != end && *r.ptr == '-') r = std::from_chars(r.ptr + 1, end, last);
            if (r.ec != std::errc{} || r.ptr != end) return false;
            if (first < 1 || last > 32 || first > last) return false;
            for (int n = first; n <= last; ++n) {
                int sq = from_pdn(n);
                pieces[c] |= bit(sq);
                if (king) kings |= bit(sq);
            }
        }
    }
    if (pieces[BLACK] & pieces[WHITE]) return false;

    set(pieces[BLACK], pieces[WHITE], kings, stm);
    return true;
}

void Position::set(Bitboard black, Bitboard white, Bitboard kings, Color stm) {
    pieces_[BLACK] = black;
    pieces_[WHITE] = white;
    kings_ = kings & (black | white);
    stm_ = stm;
    reversible_ = 0;
    key_ = compute_key();
}

Key Position::compute_key() const {
    Key k = stm_ == WHITE ? zobrist::SIDE : 0;
    for (Color c : {BLACK, WHITE})
        for (Bitboard b = pieces_[c]; b;) {
            int sq = pop_lsb(b);
            k ^= zobrist::PSQ[2 * c + ((kings_ >> sq) & 1)][sq];
        }
    return k;
}

std::string Position::fen() const {
    std::string s(1, stm_ == BLACK ? 'B' : 'W');
    for (Color c : {WHITE, BLACK}) {
        s += c == WHITE ? ":W" : ":B";
        bool first = true;
        for (int n = 1; n <= 32; ++n) {
            int sq = from_pdn(n);
            if (!(pieces_[c] & bit(sq))) continue;
            if (!first) s += ',';
            if (kings_ & bit(sq)) s += 'K';
            s += std::to_string(n);
            first = false;
        }
    }
    return s;
}

void Position::make_move(const Move& m) {
    const Color us = stm_;
    const Bitboard from = bit(m.from), to = bit(m.to);
    const bool king = kings_ & from;
    const bool crowns = !king && (to & promo_row(us));

    pieces_[us] ^= from ^ to;  // from == to for a king capturing round a circuit
    if (king) kings_ ^= from ^ to;
    key_ ^= zobrist::PSQ[2 * us + king][m.from] ^ zobrist::PSQ[2 * us + (king || crowns)][m.to];

    for (Bitboard c = m.captures; c;) {
        int sq = pop_lsb(c);
        key_ ^= zobrist::PSQ[2 * ~us + ((kings_ >> sq) & 1)][sq];
    }
    pieces_[~us] &= ~m.captures;
    kings_ &= ~m.captures;
    if (crowns) kings_ |= to;

    reversible_ = (king && !m.captures) ? reversible_ + 1 : 0;
    stm_ = ~us;
    key_ ^= zobrist::SIDE;
}

Key Position::key_after(const Move& m) const {
    const Color us = stm_;
    const bool king = kings_ & bit(m.from);
    const bool crowns = !king && (bit(m.to) & promo_row(us));
    Key k = key_ ^ zobrist::SIDE ^ zobrist::PSQ[2 * us + king][m.from] ^ zobrist::PSQ[2 * us + (king || crowns)][m.to];
    for (Bitboard c = m.captures; c;) {
        int sq = pop_lsb(c);
        k ^= zobrist::PSQ[2 * ~us + ((kings_ >> sq) & 1)][sq];
    }
    return k;
}

std::string Position::pretty() const {
    std::string s;
    for (int row = 7; row >= 0; --row) {
        s += std::to_string(row + 1) + " ";
        for (int col = 0; col < 8; ++col) {
            if ((row + col) % 2 != 0) {
                s += " .";
                continue;
            }
            int sq = row * 4 + col / 2;
            char ch = '-';
            if (pieces_[BLACK] & bit(sq)) ch = (kings_ & bit(sq)) ? 'B' : 'b';
            if (pieces_[WHITE] & bit(sq)) ch = (kings_ & bit(sq)) ? 'W' : 'w';
            s += ' ';
            s += ch;
        }
        s += '\n';
    }
    s += "   a b c d e f g h\n";
    s += std::string(stm_ == BLACK ? "Black" : "White") + " to move\n";
    return s;
}

std::string move_to_string(const Move& m) {
    return std::to_string(to_pdn(m.from)) + (m.is_capture() ? "x" : "-") + std::to_string(to_pdn(m.to));
}

}  // namespace regicide
