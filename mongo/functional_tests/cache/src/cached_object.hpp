#pragma once

#include <string>

namespace functional_tests {

struct CachedObject {
    std::string key;
    int value{0};
};

}  // namespace functional_tests
