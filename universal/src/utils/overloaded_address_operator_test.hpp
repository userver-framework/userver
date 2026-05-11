#pragma once

#include <functional>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

struct OverloadedAddressOperator {
    int payload{0};

    auto operator<=>(const OverloadedAddressOperator&) const = default;

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

template <>
struct std::hash<USERVER_NAMESPACE::utils::OverloadedAddressOperator> {
    std::size_t operator()(USERVER_NAMESPACE::utils::OverloadedAddressOperator o) const noexcept {
        return std::hash<int>{}(o.payload);
    }
};
