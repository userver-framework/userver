#pragma once

#include <cstdint>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// SipHash13 implementation.
class SipHasher final {
 public:
  SipHasher(std::uint64_t k0, std::uint64_t k1) noexcept;

  std::uint64_t operator()(std::string_view data) const noexcept;

 private:
  const std::uint64_t k0_;
  const std::uint64_t k1_;
};

// SipHash13 implementation with uppercase ASCII symbols ('A' - 'Z') being
// treated as their lowercase counterpart.
class CaseInsensitiveSipHasher final {
 public:
  CaseInsensitiveSipHasher(std::uint64_t k0, std::uint64_t k1) noexcept;

  std::uint64_t operator()(std::string_view data) const noexcept;

 private:
  const std::uint64_t k0_;
  const std::uint64_t k1_;
};

// SipHash13 implementation with uppercase ASCII symbols ('A' - 'Z') being
// treated as their lowercase counterpart.
// Same as CaseInsensitiveSipHasher, but doesn't rely on SSE2.
class CaseInsensitiveSipHasherNoSse final {
 public:
  CaseInsensitiveSipHasherNoSse(std::uint64_t k0, std::uint64_t k1) noexcept;

  std::uint64_t operator()(std::string_view data) const noexcept;

 private:
  const std::uint64_t k0_;
  const std::uint64_t k1_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
