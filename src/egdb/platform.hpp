#pragma once
// Thin OS layer (POSIX and Windows) so the database builds and probes on macOS, Linux and Windows.

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

// Slice files are little-endian with fixed-width fields; all supported hosts are little-endian.
static_assert(std::endian::native == std::endian::little, "egdb file format assumes a little-endian host");

#if defined(__GNUC__) || defined(__clang__)
#define EGDB_PREFETCH_W(p) __builtin_prefetch((p), 1)
#define EGDB_PRINTF(f, a) __attribute__((format(printf, f, a)))
#else
#define EGDB_PREFETCH_W(p) ((void)0)
#define EGDB_PRINTF(f, a)
#endif

namespace regicide::egdb::detail {

struct MappedFile {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    void* handle = nullptr;  // Windows mapping handle
};

bool map_file(const std::string& path, MappedFile& out);  // read-only, whole file
void unmap_file(MappedFile& f);
void prefault(const MappedFile& f);                       // bring all pages into RAM
bool flush_to_disk(std::FILE* f);                         // fflush + fsync
std::tm local_time(std::time_t t);

}  // namespace regicide::egdb::detail
