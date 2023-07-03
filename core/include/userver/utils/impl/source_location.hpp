#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

#include <userver/logging/log_filepath.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

class SourceLocation final {
  struct EmplaceEnabler final {
    constexpr explicit EmplaceEnabler() noexcept = default;
  };

 public:
  static constexpr SourceLocation Current(
      EmplaceEnabler /*unused*/ = EmplaceEnabler{},
      std::string_view file_name =
          std::string_view{&__builtin_FILE()[std::integral_constant<
              size_t, logging::impl::PathBaseSize(__builtin_FILE())>::value]},
      std::string_view function_name = __builtin_FUNCTION(),
      std::uint_least32_t line = __builtin_LINE()) noexcept {
    return SourceLocation{line, file_name, function_name};
  }

  // Mainly intended for interfacing with external libraries.
  static constexpr SourceLocation Custom(
      std::uint_least32_t line, std::string_view file_name,
      std::string_view function_name) noexcept {
    return SourceLocation{line, file_name, function_name};
  }

  constexpr std::uint_least32_t GetLine() const noexcept { return line_; }

  constexpr std::string_view GetFileName() const noexcept { return file_name_; }

  constexpr std::string_view GetFunctionName() const noexcept {
    return function_name_;
  }

 private:
  constexpr SourceLocation(std::uint_least32_t line, std::string_view file_name,
                           std::string_view function_name) noexcept
      : line_(line), file_name_(file_name), function_name_(function_name) {}

  std::uint_least32_t line_;
  std::string_view file_name_;
  std::string_view function_name_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
