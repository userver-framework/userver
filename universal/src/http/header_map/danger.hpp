#pragma once

#include <userver/http/predefined_header.hpp>

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace http::headers::header_map {

class Danger final {
 public:
  std::size_t HashKey(std::string_view key) const noexcept;
  std::size_t HashKey(const PredefinedHeader& header) const noexcept;

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
  std::uint64_t k0_{0};
  std::uint64_t k1_{0};
};

}  // namespace http::headers::header_map

USERVER_NAMESPACE_END
