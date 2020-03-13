#include <utils/text.hpp>

#include <unordered_map>

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

#include <engine/shared_mutex.hpp>
#include <utils/assert.hpp>

namespace utils::text {

std::string Trim(const std::string& str) {
  return boost::algorithm::trim_copy(str);
}

std::string Trim(std::string&& str) {
  boost::algorithm::trim(str);
  return std::move(str);
}

std::vector<std::string> Split(const std::string& str, const std::string& sep) {
  std::vector<std::string> result;
  boost::algorithm::split(result, str, boost::algorithm::is_any_of(sep),
                          boost::token_compress_on);
  return result;
}

std::string Join(const std::vector<std::string>& strs, const std::string& sep) {
  return boost::algorithm::join(strs, sep);
}

std::string Format(double value, const std::string& locale,
                   unsigned int ndigits, bool is_fixed) {
  std::stringstream res;
  res.imbue(GetLocale(locale));
  if (is_fixed) res.setf(std::ios::fixed, std::ios::floatfield);
  res.precision(ndigits);

  res << boost::locale::as::number << value;
  return res.str();
}

std::string Format(boost::multiprecision::cpp_dec_float_50 value,
                   unsigned int ndigits) {
  std::stringstream res;
  res << std::setprecision(ndigits) << value;
  return res.str();
}

std::string Format(double value, unsigned int ndigits) {
  std::stringstream res;
  res << std::setprecision(ndigits) << value;
  return res.str();
}

std::string Capitalize(const std::string& str, const std::string& locale) {
  return boost::locale::to_title(str, GetLocale(locale));
}

std::string RemoveQuotes(const std::string& str) {
  if (str.empty()) return std::string();
  if (str.front() != '"' || str.back() != '"') return str;
  return str.substr(1, str.size() - 2);
}

bool IsAscii(const std::string& text) {
  return std::all_of(text.cbegin(), text.cend(),
                     [](int c) { return (c >= 0 && c <= 127); });
}

const std::locale& GetLocale(const std::string& name) {
  static engine::SharedMutex m;
  using locales_map_t = std::unordered_map<std::string, std::locale>;
  static locales_map_t locales;
  {
    std::shared_lock read_lock(m);
    auto it = static_cast<const locales_map_t&>(locales).find(name);
    if (it != locales.cend()) {
      return it->second;
    }
  }

  boost::locale::generator gen;
  std::locale loc = gen(name);
  {
    std::unique_lock write_lock(m);
    return locales.emplace(name, std::move(loc)).first->second;
  }
}

namespace utf8 {

namespace {
// http://www.unicode.org/versions/latest/
//
// Unicode Encoding Forms
// Table 3-7. Well-Formed UTF-8 Byte Sequences
struct InclusiveRange {
  unsigned char from;
  unsigned char to;
};

constexpr InclusiveRange kWellFormed1Bytes[][1] = {
    {{0x00, 0x7f}},
};

constexpr InclusiveRange kWellFormed2Bytes[][2] = {
    {{0xc2, 0xdf}, {0x80, 0xbf}},
};

constexpr InclusiveRange kWellFormed3Bytes[][3] = {
    {{0xe0, 0xe0}, {0xa0, 0xbf}, {0x80, 0xbf}},
    {{0xe1, 0xec}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xed, 0xed}, {0x80, 0x9f}, {0x80, 0xbf}},
    {{0xee, 0xef}, {0x80, 0xbf}, {0x80, 0xbf}},
};

constexpr InclusiveRange kWellFormed4Bytes[][4] = {
    {{0xf0, 0xf0}, {0x90, 0xbf}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xf1, 0xf3}, {0x80, 0xbf}, {0x80, 0xbf}, {0x80, 0xbf}},
    {{0xf4, 0xf4}, {0x80, 0x8f}, {0x80, 0xbf}, {0x80, 0xbf}},
};

constexpr unsigned char kMinFirstByteValueFor2ByteCodePoint =
    kWellFormed2Bytes[0][0].from;
constexpr unsigned char kMinFirstByteValueFor3ByteCodePoint =
    kWellFormed3Bytes[0][0].from;
constexpr unsigned char kMinFirstByteValueFor4ByteCodePoint =
    kWellFormed4Bytes[0][0].from;

template <std::size_t Rows, std::size_t Columns>
bool IsValidBytes(const unsigned char* bytes,
                  const InclusiveRange (&range)[Rows][Columns]) {
  for (std::size_t row = 0; row < Rows; ++row) {
    bool valid = true;
    for (std::size_t column = 0; column < Columns; ++column) {
      const InclusiveRange table_value = range[row][column];
      const auto byte = bytes[column];

      if (table_value.from > byte || table_value.to < byte) {
        valid = false;
        break;
      }
    }

    if (valid) {
      return true;
    }
  }

  return false;
}

}  // namespace

unsigned CodePointLengthByFirstByte(unsigned char c) {
  if (c < kMinFirstByteValueFor2ByteCodePoint) {
    return 1;
  } else if (c < kMinFirstByteValueFor3ByteCodePoint) {
    return 2;
  } else if (c < kMinFirstByteValueFor4ByteCodePoint) {
    return 3;
  }

  return 4;
}

bool IsWellFormedCodePoint(const unsigned char* bytes, std::size_t length) {
  UASSERT(bytes);
  UASSERT(length != 0);

  const auto code_point_length = CodePointLengthByFirstByte(bytes[0]);
  if (length < code_point_length) {
    return false;
  }

  switch (code_point_length) {
    case 1:
      return IsValidBytes(bytes, utf8::kWellFormed1Bytes);
    case 2:
      return IsValidBytes(bytes, utf8::kWellFormed2Bytes);
    case 3:
      return IsValidBytes(bytes, utf8::kWellFormed3Bytes);
    case 4:
      return IsValidBytes(bytes, utf8::kWellFormed4Bytes);
    default:
      UASSERT_MSG(false, "Invalid UTF-8 code point length");
  }

  return false;
}

bool IsValid(const unsigned char* bytes, std::size_t length) {
  for (size_t i = 0; i < length; ++i) {
    if (!IsWellFormedCodePoint(bytes + i, length - i)) {
      return false;
    } else {
      i += CodePointLengthByFirstByte(bytes[i]) - 1;
    }
  }

  return true;
}

std::size_t GetCodePointsCount(const std::string& text) {
  const auto* const bytes = reinterpret_cast<const unsigned char*>(text.data());
  const auto size = text.size();
  std::size_t count = 0;
  for (std::size_t i = 0; i < size; i += CodePointLengthByFirstByte(bytes[i])) {
    if (!IsWellFormedCodePoint(bytes + i, size - i)) {
      throw std::runtime_error("GetCodePointsCount: text is not in utf-8");
    }
    count++;
  }
  return count;
}

void TrimTruncatedEnding(std::string& str) {
  if (str.empty()) return;

  // check if `str` ends with 1-byte character
  if (!(str.back() & 0x80)) return;

  // no symbol with a length > 4 => no proper prefix with a length > 3
  size_t max_suffix_len = std::min(str.size(), 3ul);
  for (size_t suffix_len = 1; suffix_len <= max_suffix_len; ++suffix_len) {
    size_t pos = str.size() - suffix_len;

    // first byte of a multibyte character
    if ((str[pos] & 0xc0) == 0xc0) {
      size_t expected_len = utf8::CodePointLengthByFirstByte(str[pos]);
      // check if current suffix is a proper prefix of some multibyte character
      if (expected_len > suffix_len) str.resize(str.size() - suffix_len);
      // else character is not truncated or `str` was not in utf-8
      return;
    }

    // Check if current byte is a continuation byte of a multibyte character.
    // If not then `str` is not a truncated utf-8 string.
    if ((str[pos] & 0xc0) != 0x80) return;
  }
}

}  // namespace utf8

bool IsUtf8(const std::string& text) {
  const auto* const bytes = reinterpret_cast<const unsigned char*>(text.data());
  const auto size = text.size();

  return utf8::IsValid(bytes, size);
}

bool IsPrintable(const std::string& text, bool ascii_only) {
  if (ascii_only) {
    return std::all_of(text.cbegin(), text.cend(),
                       [](int c) { return (c >= 32 && c <= 126); });
  }

  return IsUtf8(text) && !std::any_of(text.cbegin(), text.cend(), [](int c) {
           return ((c >= 0 && c <= 31) || c == 127);
         });
}

bool IsCString(const std::string& text) {
  return text.find('\0') == std::string::npos;
}

std::string CamelCaseToSnake(const std::string& camel) {
  std::string snake;

  if (!camel.empty()) {
    snake += static_cast<char>(std::tolower(camel[0]));
    for (size_t i = 1; i < camel.size(); ++i) {
      if (std::isupper(camel[i])) {
        snake += '_';
        snake += static_cast<char>(std::tolower(camel[i]));
      } else {
        snake += camel[i];
      }
    }
  }

  return snake;
}

}  // namespace utils::text
