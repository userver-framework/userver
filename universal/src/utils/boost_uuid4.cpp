#include <userver/utils/boost_uuid4.hpp>

#include <array>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

// Functions are smart copy-paste from boost::uuids::string_generator as of 1.66
// that doesn't have a bug

char GetNextChar(const char*& begin, const char* end) {
  if (begin == end) {
    throw std::runtime_error{"Invalid uuid string"};
  }
  return *begin++;
}

bool IsOpenBrace(char c) { return c == '{'; }

void CheckCloseBrace(char c, char open_brace) {
  if (open_brace == '{' && c == '}') {
    // great
  } else {
    throw std::runtime_error{"Invalid uuid string"};
  }
}

bool IsDash(char c) { return c == '-'; }

unsigned char GetValue(char c) {
  static constexpr std::string_view kDigits = "0123456789abcdefABCDEF";
  static constexpr std::array<unsigned char, kDigits.size()> kValues = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
      11, 12, 13, 14, 15, 10, 11, 12, 13, 14, 15};

  const auto pos = kDigits.find(c);
  if (pos == std::string_view::npos) {
    throw std::runtime_error{"Invalid uuid string"};
  }
  return kValues[pos];
}

boost::uuids::uuid FromChars(const char* begin, const char* end) {
  auto c = GetNextChar(begin, end);
  bool has_open_brace = IsOpenBrace(c);
  auto open_brace_char = c;
  if (has_open_brace) {
    c = GetNextChar(begin, end);
  }

  bool has_dashes = false;

  boost::uuids::uuid u{};
  int byte_no = 0;
  for (auto* byte_it = u.begin(); byte_it != u.end(); ++byte_it, ++byte_no) {
    if (byte_it != u.begin()) {
      c = GetNextChar(begin, end);
    }

    if (byte_no == 4) {
      has_dashes = IsDash(c);
      if (has_dashes) {
        c = GetNextChar(begin, end);
      }
    } else if (byte_no == 6 || byte_no == 8 || byte_no == 10) {
      if (has_dashes) {
        // Dashes must be consistent
        if (IsDash(c)) {
          c = GetNextChar(begin, end);
        } else {
          throw std::runtime_error{"Invalid uuid string"};
        }
      }
    }

    *byte_it = GetValue(c);
    c = GetNextChar(begin, end);
    *byte_it <<= 4;
    *byte_it |= GetValue(c);
  }

  if (has_open_brace) {
    c = GetNextChar(begin, end);
    CheckCloseBrace(c, open_brace_char);
  }

  if (begin != end) {
    throw std::runtime_error{"Invalid uuid string"};
  }

  return u;
}

}  // namespace

namespace generators {

boost::uuids::uuid GenerateBoostUuid() {
  thread_local boost::uuids::basic_random_generator<RandomBase> generator(
      DefaultRandom());
  return generator();
}

}  // namespace generators

boost::uuids::uuid BoostUuidFromString(std::string const& str) {
  return FromChars(str.data(), str.data() + str.size());
}

std::string ToString(const boost::uuids::uuid& uuid) {
  return boost::uuids::to_string(uuid);
}

}  // namespace utils

USERVER_NAMESPACE_END
