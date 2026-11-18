#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

inline void FillUuidRandomBytes(std::span<std::uint8_t> bytes, RandomImpl& rng) {
    while (!bytes.empty()) {
        const auto word = rng();
        const auto count = std::min(bytes.size(), sizeof(word));
        std::memcpy(bytes.data(), &word, count);
        bytes = bytes.subspan(count);
    }
}

inline void FillUuidRandomBytes(std::span<std::uint8_t> bytes) {
    auto rng_scope = utils::impl::UseLocalRandomImpl();
    // Filling the span cannot suspend, so the same thread-local generator is valid for the whole loop.
    utils::impl::FillUuidRandomBytes(bytes, *rng_scope);
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
