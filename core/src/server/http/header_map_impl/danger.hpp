#pragma once

#include <server/http/header_map_impl/traits.hpp>

#include <server/http/header_map_impl/special_header.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

class Danger final {
 public:
  std::size_t HashKey(std::string_view key) const noexcept;
  std::size_t HashKey(http::SpecialHeader header) const noexcept;

  bool IsGreen() const noexcept;
  bool IsYellow() const noexcept;
  bool IsRed() const noexcept;

  void ToGreen() noexcept;
  void ToYellow() noexcept;
  void ToRed() noexcept;

 private:
  std::size_t SafeHash(std::string_view key) const noexcept;
  static std::size_t UnsafeHash(std::string_view key) noexcept;

  enum class State { kGreen, kYellow, kRed };

  State state_{State::kGreen};
  std::size_t hash_seed_{0};
};

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
