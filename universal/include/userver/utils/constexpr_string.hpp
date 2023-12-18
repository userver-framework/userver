#pragma once
#include <algorithm>
#include <string>
USERVER_NAMESPACE_BEGIN

namespace utils {

template <std::size_t Size>
struct ConstexprString {

  static constexpr auto kSize = Size;
  constexpr operator std::string_view() const {
    return std::string_view{this->contents_.data(), Size - 1};
  };
  constexpr auto c_str() const -> const char* {
    return this->contents_.data();
  };
  constexpr std::size_t size() const {
    return Size;
  };
  constexpr const char* data() const {
    return this->contents_.begin();
  };

  constexpr operator char&() const {
    return this->contents_.data();
  };

  friend constexpr bool operator==(const ConstexprString& string, const char* other) {
    return std::string_view(string) == std::string_view(other);
  };
  template <typename Stream>
  friend constexpr auto& operator<<(Stream& stream, const ConstexprString& string) {
    stream << static_cast<std::string_view>(string);
    return stream;
  };

  constexpr ConstexprString(const char (&str)[Size]) noexcept {
    std::copy_n(str, Size, std::begin(this->contents_));
  };
  constexpr ConstexprString(std::array<char, Size> data) noexcept : contents_(data) {};
  template <std::size_t OtherSize>
  constexpr ConstexprString<Size + OtherSize> operator+(const ConstexprString<OtherSize>& other) const noexcept {
    return
        [&]<auto... I, auto... I2>(std::index_sequence<I...>, std::index_sequence<I2...>){
      return ConstexprString<Size + OtherSize>({this->contents_[I]..., other.contents_[I2]...});
    }(std::make_index_sequence<Size>(), std::make_index_sequence<OtherSize>());
  };
  template <std::size_t OtherSize>
  constexpr ConstexprString<Size + OtherSize> operator+(const char(&str)[OtherSize]) const noexcept {
    return *this + ConstexprString<OtherSize>{str};
  };
  std::array<char, Size> contents_;
};
template <std::size_t n>
ConstexprString(char const (&)[n]) -> ConstexprString<n>;
} // namespace utils
USERVER_NAMESPACE_END

