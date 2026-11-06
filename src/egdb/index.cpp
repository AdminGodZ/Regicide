#include "egdb/index.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace regicide::egdb::detail {

const std::uint32_t* unrank_table(int k) {
    static std::once_flag once[MAX_SIDE + 1];
    static std::vector<std::uint32_t> tables[MAX_SIDE + 1];
    std::call_once(once[k], [k] {
        std::vector<std::uint32_t>& t = tables[k];
        t.reserve(binom(32, k));
        if (k == 0) {
            t.push_back(0);
            return;
        }
        // Gosper's hack enumerates k-subsets in increasing numeric order, which is colex order.
        std::uint64_t v = (1ull << k) - 1;
        while (v < (1ull << 32)) {
            t.push_back(std::uint32_t(v));
            const std::uint64_t c = v & -v, r = v + c;
            v = (((r ^ v) >> 2) / c) | r;
        }
    });
    return tables[k].data();
}

}  // namespace regicide::egdb::detail
