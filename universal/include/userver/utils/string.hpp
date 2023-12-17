
#pragma once
#include <algorithm>
#include <string>
USERVER_NAMESPACE_BEGIN

namespace utils {

template <std::size_t Size = 1>
struct ConstexprString {
  static constexpr auto kSize = Size;
  std::array<char, Size> contents;
  constexpr operator std::string_view() const {
    return std::string_view{this->contents.data(), Size - 1};
  };
  constexpr auto c_str() const -> const char* {
    return this->contents.data();
  };
  constexpr const char* data() const {
    return this->contents.begin();
  };

  constexpr const char& operator[](size_t index) const {
    return this->contents[index];
  };

  constexpr operator char&() const {
    return this->contents.data();
  };

  constexpr char& operator[](size_t index) {
    return this->contents[index];
  };
  friend constexpr bool operator==(const ConstexprString& string, const char* other) {
    return std::string_view(string) == std::string_view(other);
  };
  template <typename Stream>
  friend constexpr auto operator<<(Stream& stream, const ConstexprString& string) {
    stream << std::string_view(string);
    return stream;
  };

  constexpr ConstexprString(const char (&str)[Size]) noexcept {
    std::copy_n(str, Size, begin(contents));
  };
  constexpr ConstexprString(std::array<char, Size> data) noexcept : contents(data) {};
  template <std::size_t OtherSize>
  constexpr ConstexprString<Size + OtherSize> operator+(const ConstexprString<OtherSize>& other) const noexcept {
    return
        [&]<auto... I, auto... I2>(std::index_sequence<I...>, std::index_sequence<I2...>){
      return ConstexprString<Size + OtherSize>({this->contents[I]..., other[I2]...});
    }(std::make_index_sequence<Size>(), std::make_index_sequence<OtherSize>());
  };
  template <std::size_t OtherSize>
  constexpr ConstexprString<Size + OtherSize> operator+(const char(&str)[OtherSize]) const noexcept {
    return *this + ConstexprString<OtherSize>{str};
  };
};
template <std::size_t n>
ConstexprString(char const (&)[n]) -> ConstexprString<n>;
} // namespace utils
USERVER_NAMESPACE_END

