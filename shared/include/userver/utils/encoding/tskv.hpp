#pragma once

/// @file userver/utils/encoding/tskv.hpp
/// @brief Encoders, decoders and helpers for TSKV representations

#include <ostream>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

constexpr inline char kTskvKeyValueSeparator = '=';
constexpr inline char kTskvPairsSeparator = '\t';
/// @}

// kKeyReplacePeriod is for logging. Elastic has a long history of problems with
// periods in TSKV keys. For more info see:
// www.elastic.co/guide/en/elasticsearch/reference/2.4/dots-in-names.html
enum class EncodeTskvMode { kKey, kValue, kKeyReplacePeriod };

template <typename T>
struct TypeNeedsEncodeTskv
    : std::integral_constant<bool, std::is_same<T, char>::value ||
                                       !std::is_arithmetic<T>::value> {};

template <typename T>
class EncodeTskvPutCharDefault final {
 public:
  void operator()(T& to, char ch) const { to << ch; }
};

template <>
class EncodeTskvPutCharDefault<std::ostream> final {
 public:
  void operator()(std::ostream& to, char ch) const { to.put(ch); }
};

template <>
class EncodeTskvPutCharDefault<std::string> final {
 public:
  void operator()(std::string& to, char ch) const { to.push_back(ch); }
};

/// @brief Encode according to the TSKV rules, but without escaping the
/// quotation mark (").
/// @{
template <typename T, typename Char,
          typename EncodeTskvPutChar = EncodeTskvPutCharDefault<T>>
typename std::enable_if<std::is_same<Char, char>::value, void>::type EncodeTskv(
    T& to, Char ch, EncodeTskvMode mode,
    const EncodeTskvPutChar& put_char = EncodeTskvPutChar()) {
  const bool is_key_encoding = (mode == EncodeTskvMode::kKey ||
                                mode == EncodeTskvMode::kKeyReplacePeriod);

  switch (ch) {
    case '\t':
      put_char(to, '\\');
      put_char(to, 't');
      break;
    case '\r':
      put_char(to, '\\');
      put_char(to, 'r');
      break;
    case '\n':
      put_char(to, '\\');
      put_char(to, 'n');
      break;
    case '\0':
      put_char(to, '\\');
      put_char(to, '0');
      break;
    case '\\':
      put_char(to, '\\');
      put_char(to, ch);
      break;
    case '=':
      if (is_key_encoding) put_char(to, '\\');
      put_char(to, ch);
      break;
    case '.':
      if (mode == EncodeTskvMode::kKeyReplacePeriod) {
        put_char(to, '_');
      } else {
        put_char(to, ch);
      }
      break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
      if (is_key_encoding) {
        put_char(to, ch | 0x20);  // ch - 'A' + 'a'
        break;
      }
      [[fallthrough]];
    default:
      put_char(to, ch);
      break;
  }
}

template <typename T, typename EncodeTskvPutChar = EncodeTskvPutCharDefault<T>>
void EncodeTskv(T& to, const std::string& str, EncodeTskvMode mode,
                const EncodeTskvPutChar& put_char = EncodeTskvPutChar()) {
  EncodeTskv(to, str.cbegin(), str.cend(), mode, put_char);
}

template <typename T, typename EncodeTskvPutChar = EncodeTskvPutCharDefault<T>>
void EncodeTskv(T& to, const char* str, EncodeTskvMode mode,
                const EncodeTskvPutChar& put_char = EncodeTskvPutChar()) {
  for (const auto* ptr = str; *ptr; ++ptr) EncodeTskv(to, *ptr, mode, put_char);
}

template <typename T, typename EncodeTskvPutChar = EncodeTskvPutCharDefault<T>,
          typename It>
void EncodeTskv(T& to, It first, It last, EncodeTskvMode mode,
                const EncodeTskvPutChar& put_char = EncodeTskvPutChar()) {
  for (auto it = first; it != last; ++it) EncodeTskv(to, *it, mode, put_char);
}

template <typename T, typename EncodeTskvPutChar = EncodeTskvPutCharDefault<T>>
void EncodeTskv(T& to, const char* str, size_t size, EncodeTskvMode mode,
                const EncodeTskvPutChar& put_char = EncodeTskvPutChar()) {
  EncodeTskv(to, str, str + size, mode, put_char);
}
/// @}

inline bool ShouldValueBeEscaped(std::string_view key) {
  for (char ch : key) {
    switch (ch) {
      case '\t':
      case '\r':
      case '\n':
      case '\0':
      case '\\':
        return true;
      default:
        break;
    }
  }
  return false;
}

inline bool ShouldKeyBeEscaped(std::string_view key) {
  for (char ch : key) {
    switch (ch) {
      case '\t':
      case '\r':
      case '\n':
      case '\0':
      case '\\':
      case '.':
      case '=':
        return true;
      default:
        if ('A' <= ch && ch <= 'Z') return true;
        break;
    }
  }
  return false;
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
