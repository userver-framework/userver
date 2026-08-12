#pragma once

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

struct OverloadedAddressOperator {
    int payload{0};

    auto operator<=>(const OverloadedAddressOperator& other) const { return payload <=> other.payload; };

    auto operator&() {  // NOLINT(google-runtime-operator)
        UASSERT(false);
        return this;
    }

    auto operator&() const {  // NOLINT(google-runtime-operator)
        UASSERT(false);
        return this;
    }
};

}  // namespace utils

USERVER_NAMESPACE_END
