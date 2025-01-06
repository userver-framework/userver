#pragma once

#include <concurrent/impl/rseq.hpp>

#ifdef USERVER_IMPL_HAS_RSEQ

#include <cstdint>

#include <boost/range/adaptor/strided.hpp>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

struct StripedArrayNode;

/// rseq operations take an array of per-core (per-virtual-CPU) `std::intptr_t`.
/// Each `std::intptr_t` must belong to a separate cache line to avoid
/// interference and excessive cache line flushing.
///
/// The simplest way to manage memory for rseq is to allocate
/// `InterferenceShield<std::intptr_t> x N_CORES`.
/// However, it wastes a lot of memory. StripedArray uses another approach:
///
/// - Allocate a buffer as follows:
///   `buffer = new std::intptr_t[kStride * N_CORES]`
/// - Logically split `buffer` into `kStride` StripedArray's
/// - i'th StripedArray owns one strided range within `buffer`:
///   `buffer[i]`, `buffer[i + kStride]`, `buffer[i + kStride * 2]`, etc.
///
/// Each StripedArray within a `buffer` satisfies the following properties:
///
/// - Different StripedArray's never intersect
/// - For each i, j in [0, kStride):
///   `striped_array_i[a]` and `striped_array_j[b]` are in the same cache line
///   iff `a == b`
///
/// This allows StripedArray to be usable in rseq operations.
class StripedArray final {
public:
    static constexpr std::size_t kStride = kDestructiveInterferenceSize / sizeof(std::intptr_t);

    StripedArray();
    ~StripedArray();

    std::intptr_t& operator[](std::size_t index) noexcept { return array_.GetBase()[kStride * index]; }

    const std::intptr_t& operator[](std::size_t index) const noexcept { return array_.GetBase()[kStride * index]; }

    auto Elements() {
        return utils::span<std::intptr_t>(array_.GetBase(), array_.GetBase() + GetRseqArraySizeUnsafe() * kStride) |
               boost::adaptors::strided(kStride);
    }

    auto Elements() const {
        return utils::span<const std::intptr_t>(
                   array_.GetBase(), array_.GetBase() + GetRseqArraySizeUnsafe() * kStride
               ) |
               boost::adaptors::strided(kStride);
    }

private:
    StripedArrayNode& node_;
    utils::NotNull<std::intptr_t*> array_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
