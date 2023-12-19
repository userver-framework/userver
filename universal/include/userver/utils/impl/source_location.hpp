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
      std::string_view file_name = USERVER_FILEPATH,
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

  constexpr std::string_view GetLineString() const noexcept {
    return std::string_view(&line_string_[kMaxLineDigits - line_digits_],
                            line_digits_);
  }

  constexpr std::string_view GetFileName() const noexcept { return file_name_; }

  constexpr std::string_view GetFunctionName() const noexcept {
    return function_name_;
  }

 private:
  static_assert(sizeof(std::uint_least32_t) == sizeof(std::uint32_t));

  static constexpr std::size_t kMaxLineDigits = 8;

  constexpr SourceLocation(std::uint_least32_t line, std::string_view file_name,
                           std::string_view function_name) noexcept
      : line_(line),
        line_digits_(DigitsBase10(line)),
        file_name_(file_name),
        function_name_(function_name) {
    FillLineString();
  }

  static constexpr std::size_t DigitsBase10(std::uint32_t line) noexcept {
    static_assert(sizeof(line) == 4);
    return 1                        //
           + (line >= 10)           //
           + (line >= 100)          //
           + (line >= 1000)         //
           + (line >= 10000)        //
           + (line >= 100000)       //
           + (line >= 1000000)      //
           + (line >= 10000000)     //
           + (line >= 100000000)    //
           + (line >= 1000000000);  //
  }

  constexpr void FillLineString() noexcept {
    constexpr std::size_t kMaxSupportedLine = 99999999;
    if (line_ > kMaxSupportedLine) {
      // Line string greater than 8 digits won't fit. Use fake line numbers
      // for 100M+ lines source files.
      line_ = kMaxSupportedLine;
      line_digits_ = kMaxLineDigits;
    }
    auto line = line_;
    line_string_[kMaxLineDigits - 1] = '0';
    for (std::size_t index = kMaxLineDigits - 1; line > 0;
         line /= 10, --index) {
      line_string_[index] = '0' + (line % 10);
    }
  }

  std::uint32_t line_;
  std::uint32_t line_digits_;
  char line_string_[kMaxLineDigits]{};
  std::string_view file_name_;
  std::string_view function_name_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
