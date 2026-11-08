#pragma once
// Internal registry of loaded slices, slice file format, and canonical-frame probing.

#include <cstdint>
#include <string>
#include <vector>

#include "egdb/board.hpp"
#include "egdb/codec.hpp"
#include "egdb/egdb.hpp"
#include "egdb/index.hpp"

namespace regicide::egdb::detail {

// ---- file format -----------------------------------------------------------------------------
// One file per slice and table kind: <kind>_<a><b><c><d>.rgdb, e.g. wld_2101.rgdb. Layout:
//   FileHeader | SubRecord[nsub] | payload (8-byte aligned arrays).
// Every field is a fixed-width little-endian integer and the structs have no implicit padding,
// so files are byte-identical across macOS/Linux/Windows builds and can be copied freely.
inline constexpr char FILE_MAGIC[8] = {'R', 'G', 'C', 'E', 'G', 'D', 'B', '1'};
enum class TableKind : std::uint32_t { Wld = 1, Mtc = 2 };

struct FileHeader {
    char magic[8];
    TableKind kind;
    std::uint8_t a, b, c, d;
    std::uint32_t nsub;
    std::uint32_t reserved0;
    std::uint64_t file_size;
    std::uint64_t checksum;  // FNV-1a over everything after the header
    std::uint64_t reserved[3];
};
static_assert(sizeof(FileHeader) == 64);

struct SubRecord {
    std::uint8_t x, y, kind, value;
    std::uint32_t reserved0;
    std::uint64_t size;
    std::uint64_t off_sup, n_sup, off_grp, n_grp, off_data, n_data;
};
static_assert(sizeof(SubRecord) == 64);

std::string slice_name(TableKind kind, Material m);  // file name without directory

// ---- runtime tables --------------------------------------------------------------------------
struct Sub {
    std::uint64_t size = 0;
    std::uint8_t kind = 0;  // 0 = absent, otherwise codec::Encoded::Kind
    std::uint8_t value = 0;
    const std::uint32_t* sup = nullptr;
    const std::uint16_t* grp = nullptr;
    const std::uint8_t* data = nullptr;
    std::uint64_t n_data = 0;
};

struct SliceTab {
    Sub sub[7][7];  // [x][y]
    bool complete = false;
};

// Registry. Mutations are not thread-safe; probes are (read-only).
inline SliceTab* g_tabs[2][8][8][8][8] = {};
inline SliceTab* slice_tab(TableKind kind, Material m) { return g_tabs[int(kind) - 1][m.a][m.b][m.c][m.d]; }
SliceTab& slice_tab_create(TableKind kind, Material m);
void register_sub(TableKind kind, Material m, int x, int y, const codec::Encoded& e);  // e must outlive use
void clear_slice(TableKind kind, Material m);
// Maps a slice file. A complete file replaces the slice's table and marks it complete; a partial
// file (`part`, used for resumable generation) only adds its subslices.
bool load_file(const std::string& path, TableKind expected_kind, Material expected, bool part = false);
bool load_dir(const std::string& dir, std::size_t cache_mb);
void prefault_within(std::size_t cache_mb);  // pre-faults more files, smallest first, up to the budget
bool check_file(const std::string& path);  // full checksum verification (e.g. after copying files)
bool slice_complete(TableKind kind, Material m);
int complete_pieces(TableKind kind);  // largest N with every slice of <= N pieces complete

// Write an encoded slice (subslices given by (x, y)). Writes to a temp file and renames.
struct SubEntry {
    int x, y;
    Sub sub;
};
bool write_slice_file(const std::string& path, TableKind kind, Material m, const std::vector<SubEntry>& subs);

// ---- probing in the canonical frame ----------------------------------------------------------
// Value of a stored (non-capture) position. Both sides must have pieces.
inline Wld lookup_wld(const Pos& p) {
    const Material m = material_of(p);
    if (m.a + m.b > MAX_SIDE || m.c + m.d > MAX_SIDE) return Wld::Unknown;
    const SliceTab* t = slice_tab(TableKind::Wld, m);
    if (!t) return Wld::Unknown;
    const int x = adv_x(p), y = adv_y(p);
    const Sub& s = t->sub[x][y];
    if (s.kind == codec::Encoded::Constant) return Wld(s.value + 1);
    if (s.kind != codec::Encoded::Packed) return Wld::Unknown;
    return Wld(codec::decode(s.sup, s.grp, s.data, index_of(make_dims(m, x, y), p)) + 1);
}

// Full value of any position, resolving pending captures by search.
Wld probe_canon(const Pos& p);

// Moves-to-conversion (see egdb.hpp) of a stored non-capture position known to be decided.
int lookup_mtc(const Pos& p);
int probe_mtc_canon(const Pos& p);

}  // namespace regicide::egdb::detail
