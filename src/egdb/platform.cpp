#include "egdb/platform.hpp"

#ifdef _WIN32
// PrefetchVirtualMemory and WIN32_MEMORY_RANGE_ENTRY need Windows 8 (mingw defaults to an older target).
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace regicide::egdb::detail {

#ifdef _WIN32

bool map_file(const std::string& path, MappedFile& out) {
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(file, &sz) || sz.QuadPart == 0) {
        CloseHandle(file);
        return false;
    }
    HANDLE map = CreateFileMappingA(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(file);
    if (!map) return false;
    void* addr = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if (!addr) {
        CloseHandle(map);
        return false;
    }
    out.data = static_cast<const std::uint8_t*>(addr);
    out.size = std::size_t(sz.QuadPart);
    out.handle = map;
    return true;
}

void unmap_file(MappedFile& f) {
    if (f.data) UnmapViewOfFile(f.data);
    if (f.handle) CloseHandle(static_cast<HANDLE>(f.handle));
    f = MappedFile{};
}

void prefault(const MappedFile& f) {
    WIN32_MEMORY_RANGE_ENTRY r{const_cast<std::uint8_t*>(f.data), f.size};
    PrefetchVirtualMemory(GetCurrentProcess(), 1, &r, 0);
    volatile std::uint8_t sink = 0;
    for (std::size_t i = 0; i < f.size; i += 4096) sink = sink + f.data[i];
}

bool flush_to_disk(std::FILE* f) { return std::fflush(f) == 0 && _commit(_fileno(f)) == 0; }

std::tm local_time(std::time_t t) {
    std::tm r{};
    localtime_s(&r, &t);
    return r;
}

#else

bool map_file(const std::string& path, MappedFile& out) {
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;
    struct stat st {};
    if (::fstat(fd, &st) != 0 || st.st_size == 0) {
        ::close(fd);
        return false;
    }
    void* addr = ::mmap(nullptr, std::size_t(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) return false;
    out.data = static_cast<const std::uint8_t*>(addr);
    out.size = std::size_t(st.st_size);
    return true;
}

void unmap_file(MappedFile& f) {
    if (f.data) ::munmap(const_cast<std::uint8_t*>(f.data), f.size);
    f = MappedFile{};
}

void prefault(const MappedFile& f) {
    ::madvise(const_cast<std::uint8_t*>(f.data), f.size, MADV_WILLNEED);
    volatile std::uint8_t sink = 0;
    for (std::size_t i = 0; i < f.size; i += 4096) sink = sink + f.data[i];
}

bool flush_to_disk(std::FILE* f) { return std::fflush(f) == 0 && ::fsync(fileno(f)) == 0; }

std::tm local_time(std::time_t t) {
    std::tm r{};
    localtime_r(&t, &r);
    return r;
}

#endif

}  // namespace regicide::egdb::detail
