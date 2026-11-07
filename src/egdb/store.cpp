#include "egdb/store.hpp"

#include "egdb/platform.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>

namespace regicide::egdb::detail {

namespace {

std::vector<std::unique_ptr<SliceTab>>& owned_tabs() {
    static std::vector<std::unique_ptr<SliceTab>> v;
    return v;
}

std::vector<MappedFile>& mappings() {
    static std::vector<MappedFile> v;
    return v;
}

std::uint64_t fnv1a(const std::uint8_t* p, std::size_t n, std::uint64_t h = 0xcbf29ce484222325ull) {
    for (std::size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

bool valid_material(Material m) {
    return m.a >= 0 && m.b >= 0 && m.c >= 0 && m.d >= 0 && m.a + m.b >= 1 && m.c + m.d >= 1 &&
           m.a + m.b <= MAX_SIDE && m.c + m.d <= MAX_SIDE;
}

}  // namespace

std::string slice_name(TableKind kind, Material m) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%s_%d%d%d%d.rgdb", kind == TableKind::Wld ? "wld" : "mtc", m.a, m.b, m.c, m.d);
    return buf;
}

SliceTab& slice_tab_create(TableKind kind, Material m) {
    SliceTab*& slot = g_tabs[int(kind) - 1][m.a][m.b][m.c][m.d];
    if (!slot) {
        owned_tabs().push_back(std::make_unique<SliceTab>());
        slot = owned_tabs().back().get();
    }
    return *slot;
}

void register_sub(TableKind kind, Material m, int x, int y, const codec::Encoded& e) {
    Sub& s = slice_tab_create(kind, m).sub[x][y];
    s.size = e.size;
    s.kind = e.kind;
    s.value = e.value;
    s.sup = e.sup.data();
    s.grp = e.grp.data();
    s.data = e.data.data();
    s.n_data = e.data.size();
}

void clear_slice(TableKind kind, Material m) {
    if (SliceTab* t = slice_tab(kind, m)) *t = SliceTab{};
}

bool slice_complete(TableKind kind, Material m) {
    const SliceTab* t = slice_tab(kind, m);
    return t && t->complete;
}

int complete_pieces(TableKind kind) {
    int best = 0;
    for (int n = 2; n <= MAX_PIECES; ++n) {
        for (int a = 0; a <= n; ++a)
            for (int b = 0; a + b <= n; ++b)
                for (int c = 0; a + b + c <= n; ++c) {
                    const Material m{a, b, c, n - a - b - c};
                    if (valid_material(m) && !slice_complete(kind, m)) return best;
                }
        best = n;
    }
    return best;
}

bool write_slice_file(const std::string& path, TableKind kind, Material m, const std::vector<SubEntry>& subs) {
    std::vector<SubRecord> recs(subs.size());
    std::uint64_t off = sizeof(FileHeader) + subs.size() * sizeof(SubRecord);
    auto align8 = [](std::uint64_t v) { return (v + 7) & ~std::uint64_t(7); };
    for (std::size_t i = 0; i < subs.size(); ++i) {
        const Sub& e = subs[i].sub;
        SubRecord& r = recs[i];
        std::memset(&r, 0, sizeof r);
        r.x = std::uint8_t(subs[i].x);
        r.y = std::uint8_t(subs[i].y);
        r.kind = e.kind;
        r.value = e.value;
        r.size = e.size;
        off = align8(off);
        r.off_sup = off;
        const bool packed = e.kind == codec::Encoded::Packed;
        r.n_grp = packed ? (e.size + codec::GROUP - 1) / codec::GROUP : 0;
        r.n_sup = packed ? (r.n_grp + (1u << codec::SUPER_SHIFT) - 1) >> codec::SUPER_SHIFT : 0;
        off += r.n_sup * 4;
        off = align8(off);
        r.off_grp = off;
        off += r.n_grp * 2;
        off = align8(off);
        r.off_data = off;
        r.n_data = packed ? e.n_data : 0;
        off += r.n_data + 8;  // slack so decoders may over-read
    }
    const std::uint64_t file_size = align8(off);
    std::vector<std::uint8_t> buf(file_size, 0);
    std::memcpy(buf.data() + sizeof(FileHeader), recs.data(), recs.size() * sizeof(SubRecord));
    for (std::size_t i = 0; i < subs.size(); ++i) {
        const Sub& e = subs[i].sub;
        if (recs[i].n_sup) std::memcpy(buf.data() + recs[i].off_sup, e.sup, recs[i].n_sup * 4);
        if (recs[i].n_grp) std::memcpy(buf.data() + recs[i].off_grp, e.grp, recs[i].n_grp * 2);
        if (recs[i].n_data) std::memcpy(buf.data() + recs[i].off_data, e.data, recs[i].n_data);
    }
    FileHeader h{};
    std::memcpy(h.magic, FILE_MAGIC, 8);
    h.kind = kind;
    h.a = std::uint8_t(m.a);
    h.b = std::uint8_t(m.b);
    h.c = std::uint8_t(m.c);
    h.d = std::uint8_t(m.d);
    h.nsub = std::uint32_t(subs.size());
    h.file_size = file_size;
    h.checksum = fnv1a(buf.data() + sizeof h, file_size - sizeof h);
    std::memcpy(buf.data(), &h, sizeof h);

    const std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    const bool ok = std::fwrite(buf.data(), 1, buf.size(), f) == buf.size() && flush_to_disk(f);
    std::fclose(f);
    if (!ok) return false;
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);  // replaces an existing file on every platform
    return !ec;
}

bool check_file(const std::string& path) {
    MappedFile mf;
    if (!map_file(path, mf)) return false;
    FileHeader h;
    bool ok = mf.size >= sizeof h;
    if (ok) {
        std::memcpy(&h, mf.data, sizeof h);
        ok = std::memcmp(h.magic, FILE_MAGIC, 8) == 0 && h.file_size == mf.size &&
             fnv1a(mf.data + sizeof h, mf.size - sizeof h) == h.checksum;
    }
    unmap_file(mf);
    return ok;
}

bool load_file(const std::string& path, TableKind expected_kind, Material expected, bool part) {
    MappedFile mf;
    if (!map_file(path, mf)) return false;
    const std::size_t len = mf.size;
    const std::uint8_t* base = mf.data;
    if (len < sizeof(FileHeader)) {
        unmap_file(mf);
        return false;
    }
    FileHeader h;
    std::memcpy(&h, base, sizeof h);
    const Material m{h.a, h.b, h.c, h.d};
    bool ok = std::memcmp(h.magic, FILE_MAGIC, 8) == 0 && h.kind == expected_kind && m == expected &&
              h.file_size == len && sizeof(FileHeader) + std::uint64_t(h.nsub) * sizeof(SubRecord) <= len;
    std::vector<SubRecord> recs;
    if (ok) {
        recs.resize(h.nsub);
        std::memcpy(recs.data(), base + sizeof h, h.nsub * sizeof(SubRecord));
        for (const SubRecord& r : recs) {
            const bool in_range = r.x < 7 && r.y < 7 && r.off_sup + r.n_sup * 4 <= len &&
                                  r.off_grp + r.n_grp * 2 <= len && r.off_data + r.n_data <= len;
            const SubDims dims = make_dims(m, r.x, r.y);
            if (!in_range || r.size != dims.size ||
                (r.kind == codec::Encoded::Packed &&
                 (r.n_grp != (r.size + codec::GROUP - 1) / codec::GROUP ||
                  r.n_sup != (r.n_grp + (1u << codec::SUPER_SHIFT) - 1) >> codec::SUPER_SHIFT)))
                ok = false;
        }
    }
    if (!ok) {
        std::fprintf(stderr, "egdb: rejecting corrupt or mismatched file %s\n", path.c_str());
        unmap_file(mf);
        return false;
    }
    mappings().push_back(mf);
    SliceTab& t = slice_tab_create(expected_kind, m);
    if (!part) t = SliceTab{};
    for (const SubRecord& r : recs) {
        Sub& s = t.sub[r.x][r.y];
        s.size = r.size;
        s.kind = r.kind;
        s.value = r.value;
        s.sup = reinterpret_cast<const std::uint32_t*>(base + r.off_sup);
        s.grp = reinterpret_cast<const std::uint16_t*>(base + r.off_grp);
        s.data = base + r.off_data;
        s.n_data = r.n_data;
    }
    if (!part) t.complete = true;
    return true;
}

bool load_dir(const std::string& dir, std::size_t cache_mb) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return false;
    std::uint64_t total = 0;
    std::vector<std::pair<std::string, std::uint64_t>> loaded;
    for (const auto& ent : fs::directory_iterator(dir, ec)) {
        const std::string name = ent.path().filename().string();
        int a, b, c, d;
        char kind[4] = {};
        if (name.size() != 13 || name.substr(9) != "rgdb" ||
            std::sscanf(name.c_str(), "%3[a-z]_%1d%1d%1d%1d.rgdb", kind, &a, &b, &c, &d) != 5)
            continue;
        const TableKind k = std::strcmp(kind, "wld") == 0 ? TableKind::Wld
                            : std::strcmp(kind, "mtc") == 0 ? TableKind::Mtc
                                                            : TableKind(0);
        const Material m{a, b, c, d};
        if (k == TableKind(0) || !valid_material(m)) continue;
        if (load_file(ent.path().string(), k, m)) {
            total += ent.file_size(ec);
            loaded.emplace_back(ent.path().string(), ent.file_size(ec));
        }
    }
    if (total <= std::uint64_t(cache_mb) << 20) {
        // Pre-fault everything so that probes never wait for the disk.
        for (const MappedFile& mp : mappings()) prefault(mp);
    }
    return !loaded.empty();
}

Wld probe_canon(const Pos& p) {
    if (!p.own()) return Wld::Loss;
    if (!p.opp()) return has_quiet_move(p) ? Wld::Win : Wld::Loss;
    if (has_capture(p)) {
        bool draw = false, unknown = false;
        const bool win = for_each_capture(p, [&](const Pos& q) {
            const Wld r = probe_canon(q);
            draw |= r == Wld::Draw;
            unknown |= r == Wld::Unknown;
            return r == Wld::Loss;
        });
        if (win) return Wld::Win;
        if (unknown) return Wld::Unknown;
        return draw ? Wld::Draw : Wld::Loss;
    }
    return lookup_wld(p);
}

int lookup_mtc(const Pos& p) {
    const Material m = material_of(p);
    if (m.a + m.b > MAX_SIDE || m.c + m.d > MAX_SIDE) return MTC_UNKNOWN;
    const SliceTab* t = slice_tab(TableKind::Mtc, m);
    if (!t) return MTC_UNKNOWN;
    const int x = adv_x(p), y = adv_y(p);
    const Sub& s = t->sub[x][y];
    if (s.kind != codec::Encoded::Packed) return MTC_UNKNOWN;
    return codec::mtc::decode(s.sup, s.grp, s.data, index_of(make_dims(m, x, y), p));
}

int probe_mtc_canon(const Pos& p) {
    const Wld w = probe_canon(p);
    if (w == Wld::Unknown) return MTC_UNKNOWN;
    if (w == Wld::Draw) return MTC_DRAW;
    if (!p.own() || !p.opp() || has_capture(p)) return 0;
    return lookup_mtc(p);
}

}  // namespace regicide::egdb::detail

namespace regicide::egdb {

namespace {
std::mutex g_init_mutex;
bool g_init_ok = false;
}  // namespace

// The first directory that loads stays in use (tables can't be unmapped under running searches); a failed
// attempt, e.g. a missing default directory, leaves init() free to try another one.
bool init(const std::string& dir, std::size_t cache_mb) {
    std::lock_guard lock(g_init_mutex);
    if (!g_init_ok) g_init_ok = detail::load_dir(dir, cache_mb);
    return g_init_ok;
}

int max_pieces() { return detail::complete_pieces(detail::TableKind::Wld); }
int max_mtc_pieces() { return detail::complete_pieces(detail::TableKind::Mtc); }

Wld probe(Bitboard black, Bitboard white, Bitboard kings, Color stm) {
    if (std::popcount(black | white) > detail::MAX_PIECES) return Wld::Unknown;
    return detail::probe_canon(detail::from_colors(black, white, kings, stm));
}

int probe_mtc(Bitboard black, Bitboard white, Bitboard kings, Color stm) {
    if (std::popcount(black | white) > detail::MAX_PIECES) return MTC_UNKNOWN;
    return detail::probe_mtc_canon(detail::from_colors(black, white, kings, stm));
}

}  // namespace regicide::egdb
