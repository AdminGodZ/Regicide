#include "openings.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace match {

namespace {

// The ACF 3-move ballot deck, in ACF order, as embedded in CheckerBoard (utility.c,
// three_move_table). Kind: 'X' cross-board (144), 'M' mail-play only (12), 'B' barred (18).
struct Ballot {
    int acf;
    const char* moves;
    char kind;
    const char* name;
};
constexpr Ballot kAcfDeck[] = {
    {1, "9-13 21-17 5-9", 'X', nullptr},
    {2, "9-13 21-17 6-9", 'X', nullptr},
    {3, "9-13 21-17 10-14", 'B', nullptr},
    {4, "9-13 22-17 13x22", 'X', nullptr},
    {5, "9-13 22-18 6-9", 'X', "Dreaded Edinburgh"},
    {6, "9-13 22-18 10-14", 'M', "Inferno"},
    {7, "9-13 22-18 10-15", 'X', nullptr},
    {8, "9-13 22-18 11-15", 'X', "Edinburgh Single"},
    {9, "9-13 22-18 11-16", 'M', "Wilderness I"},
    {10, "9-13 22-18 12-16", 'X', nullptr},
    {11, "9-13 23-18 5-9", 'X', nullptr},
    {12, "9-13 23-18 6-9", 'X', nullptr},
    {13, "9-13 23-18 10-14", 'B', nullptr},
    {14, "9-13 23-18 10-15", 'X', nullptr},
    {15, "9-13 23-18 11-15", 'X', nullptr},
    {16, "9-13 23-18 11-16", 'M', "Wilderness II"},
    {17, "9-13 23-18 12-16", 'X', nullptr},
    {18, "9-13 23-19 5-9", 'X', nullptr},
    {19, "9-13 23-19 6-9", 'X', nullptr},
    {20, "9-13 23-19 10-14", 'X', nullptr},
    {21, "9-13 23-19 10-15", 'M', nullptr},
    {22, "9-13 23-19 11-16", 'X', nullptr},
    {23, "9-13 24-19 5-9", 'X', nullptr},
    {24, "9-13 24-19 6-9", 'X', nullptr},
    {25, "9-13 24-19 10-14", 'X', nullptr},
    {26, "9-13 24-19 10-15", 'B', nullptr},
    {27, "9-13 24-19 11-15", 'X', nullptr},
    {28, "9-13 24-19 11-16", 'X', nullptr},
    {29, "9-13 24-20 5-9", 'X', nullptr},
    {30, "9-13 24-20 6-9", 'X', nullptr},
    {31, "9-13 24-20 10-14", 'X', nullptr},
    {32, "9-13 24-20 10-15", 'X', nullptr},
    {33, "9-13 24-20 11-15", 'X', nullptr},
    {34, "9-13 24-20 11-16", 'M', "Twilight Zone"},
    {35, "9-13 24-20 12-16", 'B', "Dundee Barred"},
    {36, "9-14 22-17 5-9", 'X', nullptr},
    {37, "9-14 22-17 6-9", 'X', nullptr},
    {38, "9-14 22-17 11-15", 'X', nullptr},
    {39, "9-14 22-17 11-16", 'X', nullptr},
    {40, "9-14 22-18 5-9", 'X', nullptr},
    {41, "9-14 22-18 10-15", 'X', nullptr},
    {42, "9-14 22-18 11-15", 'X', nullptr},
    {43, "9-14 22-18 11-16", 'X', nullptr},
    {44, "9-14 22-18 12-16", 'B', nullptr},
    {45, "9-14 22-18 14-17", 'B', nullptr},
    {46, "9-14 23-18 14x23", 'X', "Double Cross"},
    {47, "9-14 23-19 5-9", 'X', nullptr},
    {48, "9-14 23-19 10-15", 'B', "Rattlesnake"},
    {49, "9-14 23-19 11-16", 'X', nullptr},
    {50, "9-14 23-19 14-18", 'X', nullptr},
    {51, "9-14 24-19 5-9", 'X', nullptr},
    {52, "9-14 24-19 10-15", 'B', nullptr},
    {53, "9-14 24-19 11-15", 'X', nullptr},
    {54, "9-14 24-19 11-16", 'X', nullptr},
    {55, "9-14 24-20 5-9", 'X', nullptr},
    {56, "9-14 24-20 10-15", 'X', nullptr},
    {57, "9-14 24-20 11-15", 'X', nullptr},
    {58, "9-14 24-20 11-16", 'X', nullptr},
    {59, "10-14 22-17 7-10", 'X', nullptr},
    {60, "10-14 22-17 9-13", 'M', "Black Hole"},
    {61, "10-14 22-17 11-15", 'M', nullptr},
    {62, "10-14 22-17 11-16", 'M', "Gemini I"},
    {63, "10-14 22-17 14-18", 'X', nullptr},
    {64, "10-14 22-18 6-10", 'X', nullptr},
    {65, "10-14 22-18 7-10", 'X', "Fraser's Inferno"},
    {66, "10-14 22-18 11-15", 'X', nullptr},
    {67, "10-14 22-18 11-16", 'X', nullptr},
    {68, "10-14 22-18 12-16", 'X', "White Doctor"},
    {69, "10-14 23-18 14x23", 'X', nullptr},
    {70, "10-14 23-19 6-10", 'X', nullptr},
    {71, "10-14 23-19 7-10", 'X', "Diabolical Denny"},
    {72, "10-14 23-19 11-15", 'X', nullptr},
    {73, "10-14 23-19 11-16", 'X', nullptr},
    {74, "10-14 23-19 14-18", 'X', nullptr},
    {75, "10-14 24-19 6-10", 'X', nullptr},
    {76, "10-14 24-19 7-10", 'X', nullptr},
    {77, "10-14 24-19 11-15", 'B', nullptr},
    {78, "10-14 24-19 11-16", 'X', nullptr},
    {79, "10-14 24-19 14-18", 'X', nullptr},
    {80, "10-14 24-20 6-10", 'X', nullptr},
    {81, "10-14 24-20 7-10", 'X', nullptr},
    {82, "10-14 24-20 11-15", 'X', nullptr},
    {83, "10-14 24-20 11-16", 'X', nullptr},
    {84, "10-14 24-20 14-18", 'X', nullptr},
    {85, "10-15 21-17 6-10", 'X', nullptr},
    {86, "10-15 21-17 7-10", 'X', "Octopus"},
    {87, "10-15 21-17 9-13", 'X', "Tyne"},
    {88, "10-15 21-17 9-14", 'B', nullptr},
    {89, "10-15 21-17 11-16", 'X', nullptr},
    {90, "10-15 21-17 15-18", 'X', nullptr},
    {91, "10-15 22-17 6-10", 'X', nullptr},
    {92, "10-15 22-17 7-10", 'X', nullptr},
    {93, "10-15 22-17 9-13", 'X', nullptr},
    {94, "10-15 22-17 9-14", 'B', "Nutcracker"},
    {95, "10-15 22-17 11-16", 'X', nullptr},
    {96, "10-15 22-17 15-19", 'X', "Skull-Cracker"},
    {97, "10-15 22-18 15x22", 'X', nullptr},
    {98, "10-15 23-18 6-10", 'X', nullptr},
    {99, "10-15 23-18 7-10", 'X', nullptr},
    {100, "10-15 23-18 9-14", 'X', nullptr},
    {101, "10-15 23-18 11-16", 'X', nullptr},
    {102, "10-15 23-18 12-16", 'X', nullptr},
    {103, "10-15 23-19 6-10", 'X', nullptr},
    {104, "10-15 23-19 7-10", 'X', nullptr},
    {105, "10-15 23-19 11-16", 'M', "Gemini II"},
    {106, "10-15 24-19 15x24", 'X', nullptr},
    {107, "10-15 24-20 6-10", 'X', nullptr},
    {108, "10-15 24-20 7-10", 'X', nullptr},
    {109, "10-15 24-20 11-16", 'B', nullptr},
    {110, "10-15 24-20 15-19", 'X', nullptr},
    {111, "11-15 21-17 8-11", 'X', nullptr},
    {112, "11-15 21-17 9-13", 'X', nullptr},
    {113, "11-15 21-17 9-14", 'X', nullptr},
    {114, "11-15 21-17 10-14", 'B', nullptr},
    {115, "11-15 21-17 15-19", 'X', nullptr},
    {116, "11-15 22-17 8-11", 'X', nullptr},
    {117, "11-15 22-17 9-13", 'X', nullptr},
    {118, "11-15 22-17 15-18", 'X', nullptr},
    {119, "11-15 22-17 15-19", 'X', nullptr},
    {120, "11-15 22-18 15x22", 'X', nullptr},
    {121, "11-15 23-18 8-11", 'X', nullptr},
    {122, "11-15 23-18 9-14", 'X', nullptr},
    {123, "11-15 23-18 10-14", 'X', nullptr},
    {124, "11-15 23-18 12-16", 'X', nullptr},
    {125, "11-15 23-18 15-19", 'X', nullptr},
    {126, "11-15 23-19 8-11", 'X', nullptr},
    {127, "11-15 23-19 9-13", 'X', nullptr},
    {128, "11-15 23-19 9-14", 'X', nullptr},
    {129, "11-15 24-19 15x24", 'X', nullptr},
    {130, "11-15 24-20 8-11", 'X', nullptr},
    {131, "11-15 24-20 12-16", 'X', nullptr},
    {132, "11-15 24-20 15-18", 'X', nullptr},
    {133, "11-16 21-17 7-11", 'X', "Octopus"},
    {134, "11-16 21-17 8-11", 'X', nullptr},
    {135, "11-16 21-17 9-13", 'X', nullptr},
    {136, "11-16 21-17 9-14", 'X', nullptr},
    {137, "11-16 21-17 10-14", 'B', "Shark"},
    {138, "11-16 21-17 16-20", 'X', nullptr},
    {139, "11-16 22-17 7-11", 'X', nullptr},
    {140, "11-16 22-17 8-11", 'X', nullptr},
    {141, "11-16 22-17 9-13", 'M', nullptr},
    {142, "11-16 22-17 16-20", 'X', nullptr},
    {143, "11-16 22-18 7-11", 'X', nullptr},
    {144, "11-16 22-18 8-11", 'X', nullptr},
    {145, "11-16 22-18 10-15", 'B', "Cheetah"},
    {146, "11-16 22-18 16-19", 'X', nullptr},
    {147, "11-16 22-18 16-20", 'X', nullptr},
    {148, "11-16 23-18 7-11", 'X', nullptr},
    {149, "11-16 23-18 8-11", 'X', nullptr},
    {150, "11-16 23-18 9-14", 'X', nullptr},
    {151, "11-16 23-18 10-14", 'X', nullptr},
    {152, "11-16 23-18 16-20", 'X', nullptr},
    {153, "11-16 23-19 16x23", 'M', "Black Widow"},
    {154, "11-16 24-19 7-11", 'X', nullptr},
    {155, "11-16 24-19 8-11", 'X', nullptr},
    {156, "11-16 24-19 10-15", 'B', nullptr},
    {157, "11-16 24-19 16-20", 'X', nullptr},
    {158, "11-16 24-20 7-11", 'X', nullptr},
    {159, "11-16 24-20 16-19", 'X', nullptr},
    {160, "12-16 21-17 9-13", 'X', nullptr},
    {161, "12-16 21-17 9-14", 'X', nullptr},
    {162, "12-16 21-17 16-19", 'X', nullptr},
    {163, "12-16 21-17 16-20", 'X', nullptr},
    {164, "12-16 22-17 16-19", 'X', nullptr},
    {165, "12-16 22-17 16-20", 'X', nullptr},
    {166, "12-16 22-18 16-19", 'X', nullptr},
    {167, "12-16 22-18 16-20", 'X', nullptr},
    {168, "12-16 23-18 9-14", 'B', nullptr},
    {169, "12-16 23-18 16-19", 'X', nullptr},
    {170, "12-16 23-18 16-20", 'X', nullptr},
    {171, "12-16 23-19 16x23", 'B', nullptr},
    {172, "12-16 24-19 16-20", 'X', nullptr},
    {173, "12-16 24-20 8-12", 'X', nullptr},
    {174, "12-16 24-20 10-15", 'M', "Skunk"},
};
static_assert(sizeof(kAcfDeck) / sizeof(kAcfDeck[0]) == 174);

std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

bool looks_like_fen(const std::string& s) {
    std::size_t i = 0;
    while (i < s.size() && (s[i] == '"' || std::isspace(static_cast<unsigned char>(s[i])))) ++i;
    return i + 1 < s.size() && (std::toupper(static_cast<unsigned char>(s[i])) == 'B' ||
                                std::toupper(static_cast<unsigned char>(s[i])) == 'W') && s[i + 1] == ':';
}

bool finish(Opening& o, std::string& err) {
    cbt::Board b = cbt::Board::start();
    if (!o.fen.empty()) {
        auto f = cbt::Board::from_fen(o.fen, &err);
        if (!f) return false;
        b = *f;
    }
    auto after = cbt::play_moves(b, o.moves, &o.played, &err);
    if (!after) {
        err = o.name + ": " + err;
        return false;
    }
    if (after->legal_moves().empty()) {
        err = o.name + ": no legal moves in the opening position";
        return false;
    }
    o.start = *after;
    return true;
}

// Strip PDN comments, variations and move numbers' trailing dots are handled by play_moves().
std::string clean_movetext(const std::string& s) {
    std::string r;
    bool comment = false;  // inside { }: PDN comments do not nest, and their text is not looked at
    int paren = 0;
    for (char c : s) {
        if (comment) comment = c != '}';
        else if (c == '{') comment = true;
        else if (c == '(') ++paren;
        else if (c == ')') paren -= paren > 0;
        else if (paren == 0) r += c;
    }
    return r;
}

std::vector<Opening> load_pdn(const std::string& text, const std::string& path, std::string& err) {
    std::vector<Opening> out;
    std::istringstream in(text);
    std::string line, fen, movetext;
    bool in_game = false;
    auto flush = [&]() -> bool {
        if (!in_game) return true;
        Opening o;
        o.name = path + ":" + std::to_string(out.size() + 1);
        o.fen = fen;
        o.moves = trim(clean_movetext(movetext));
        if (!finish(o, err)) return false;
        out.push_back(std::move(o));
        fen.clear();
        movetext.clear();
        in_game = false;
        return true;
    };
    bool in_tags = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line[0] == '%') continue;  // PDN: a line starting with '%' is a comment
        std::string t = trim(line);
        if (!t.empty() && t[0] == '[') {
            if (!in_tags && in_game && !flush()) return {};
            in_tags = in_game = true;
            if (t.rfind("[FEN", 0) == 0) {
                auto q1 = t.find('"'), q2 = t.rfind('"');
                if (q1 != std::string::npos && q2 > q1) fen = t.substr(q1 + 1, q2 - q1 - 1);
            }
            continue;
        }
        in_tags = false;
        if (!t.empty()) {
            in_game = true;
            movetext += " " + t;
        }
    }
    if (!flush()) return {};
    return out;
}

}  // namespace

std::vector<Opening> load_openings(const std::string& deck, std::string& err) {
    std::vector<Opening> out;
    if (deck == "acf156" || deck == "acf174" || deck == "acf144") {
        for (const Ballot& b : kAcfDeck) {
            if (deck == "acf144" && b.kind != 'X') continue;
            if (deck == "acf156" && b.kind == 'B') continue;
            Opening o;
            o.name = "ACF #" + std::to_string(b.acf);
            if (b.name) o.name += std::string(" ") + b.name;
            o.moves = b.moves;
            if (!finish(o, err)) return {};
            out.push_back(std::move(o));
        }
        return out;
    }

    std::ifstream f(deck);
    if (!f) {
        err = "cannot open openings file '" + deck + "' (or unknown deck; use acf156, acf174, acf144)";
        return {};
    }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();
    {  // PDN if the first thing in the file, after any '%' comment lines, is a tag
        std::istringstream probe(text);
        std::string first;
        while (std::getline(probe, first)) {
            if (!first.empty() && first[0] == '%') continue;
            first = trim(first);
            if (first.empty()) continue;
            if (first[0] == '[') return load_pdn(text, deck, err);
            break;
        }
    }

    std::istringstream in(text);
    std::string line;
    int lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        line = trim(line);
        if (line.empty()) continue;
        Opening o;
        o.name = deck + ":" + std::to_string(lineno);
        auto bar = line.find('|');
        if (bar != std::string::npos) {
            o.fen = trim(line.substr(0, bar));
            o.moves = trim(line.substr(bar + 1));
            if (auto bar2 = o.moves.find('|'); bar2 != std::string::npos) {
                const std::string side = trim(o.moves.substr(bar2 + 1));
                o.moves = trim(o.moves.substr(0, bar2));
                if (side == "A=B" || side == "A=W") {
                    o.a_colour = side[2];
                } else {
                    err = deck + ":" + std::to_string(lineno) + ": third field must be A=B or A=W";
                    return {};
                }
            }
        } else if (looks_like_fen(line)) {
            o.fen = line;
        } else {
            o.moves = line;
        }
        if (!o.fen.empty() && o.fen.front() == '"') o.fen = trim(o.fen.substr(1, o.fen.find('"', 1) - 1));
        if (!finish(o, err)) return {};
        out.push_back(std::move(o));
    }
    if (out.empty()) err = "no openings in '" + deck + "'";
    return out;
}

}  // namespace match
