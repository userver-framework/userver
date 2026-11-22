#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <userver/chaotic/convert/to.hpp>

namespace test {

/// An explicit wrapper over the items of an array, converted as a whole
class Counts final {
public:
    Counts() = default;
    explicit Counts(std::vector<std::int64_t> values) : values_(std::move(values)) {}

    const std::vector<std::int64_t>& GetValues() const { return values_; }

    bool operator==(const Counts&) const = default;

private:
    std::vector<std::int64_t> values_;
};

inline Counts Convert(std::vector<std::int64_t>&& values, USERVER_NAMESPACE::chaotic::convert::To<Counts>) {
    return Counts{std::move(values)};
}

inline std::vector<std::int64_t> Convert(
    const Counts& counts,
    USERVER_NAMESPACE::chaotic::convert::To<std::vector<std::int64_t>>
) {
    return counts.GetValues();
}

}  // namespace test
