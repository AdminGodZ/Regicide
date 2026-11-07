// The built-in weights must match the compiled feature set; otherwise the engine silently falls back to the
// hand-crafted evaluation.
#include <iostream>

#include "eval.hpp"

int main() {
    if (!regicide::Evaluator::builtin()->trained()) {
        std::cerr << "built-in eval weights do not match the feature set (features::COUNT = "
                  << regicide::features::COUNT << ")\n";
        return 1;
    }
    std::cout << "built-in eval weights loaded\n";
    return 0;
}
