#pragma once
// A tiny RGNN net for tests: version 1, FT 128, L1 16, L2 16, one bucket, every weight zero. Its evaluation is the
// output bias scaled by out_scale, so a constant score is one field away.
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "eval.hpp"

namespace test_net {

// Byte offsets in the file (see the format in nnue.hpp).
constexpr std::size_t OUT_SCALE = 40;     // u32, last header field
constexpr std::size_t L1_BIAS = 33068;    // i32[16], after the header, ft_bias and ft_weight
constexpr std::size_t L2_BIAS = 35180;    // i32[16], after l1_bias and l1_weight
constexpr std::size_t OUT_BIAS = 35500;   // i32[1], after l2_bias and l2_weight
constexpr std::size_t OUT_WEIGHT = 35504; // i16[16]
constexpr std::size_t SIZE = 35536;

template <class T>
void put(std::vector<unsigned char>& d, std::size_t offset, T v) {
    std::memcpy(d.data() + offset, &v, sizeof v);
}

inline std::vector<unsigned char> bytes() {
    std::vector<unsigned char> d(SIZE);
    std::memcpy(d.data(), "RGNN", 4);
    // version, inputs, ft, l1, l2, buckets, activation, qa, qb, out_scale
    const std::uint32_t header[] = {1, 128, 128, 16, 16, 1, 1, 255, 64, 400};
    std::memcpy(d.data() + 4, header, sizeof header);
    return d;
}

// An evaluator that scores every position `cm` for the side to move (through Evaluator::from_file).
inline std::shared_ptr<const regicide::Evaluator> constant(int cm) {
    auto d = bytes();
    put<std::int32_t>(d, OUT_BIAS, cm * 8128 / 400);  // score = out * out_scale / (127 * 64)
    const auto path = std::filesystem::temp_directory_path() /
                      ("regicide-test-net-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".nnue");
    {
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(d.data()), std::streamsize(d.size()));
    }
    auto ev = regicide::Evaluator::from_file(path.string());
    std::filesystem::remove(path);
    return ev;
}

}  // namespace test_net
