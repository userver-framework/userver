#pragma once

/// @file userver/crypto/random.hpp
/// @brief Cryptographically-secure randomness

#include <cstddef>
#include <string>
#include <type_traits>

#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

namespace impl {

void GenerateRandomBlock(utils::span<std::byte> buffer);

}  // namespace impl

/// @brief Generate a block of randomness using a cryptographically secure RNG.
///
/// Uses a thread-local CryptoPP::AutoSeededRandomPool.
void GenerateRandomBlock(utils::span<char> buffer);

/// @overload
template <typename T>
void GenerateRandomBlock(utils::span<T> buffer) {
  static_assert(std::is_trivially_copyable_v<T>);
  impl::GenerateRandomBlock(utils::as_writable_bytes(buffer));
}

/// @overload
std::string GenerateRandomBlock(std::size_t size);

}  // namespace crypto

USERVER_NAMESPACE_END
