#pragma once

#include <strings.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace utils {

class StrIcaseHash final {
 public:
  std::size_t operator()(const std::string& s) const {
    std::size_t res = 0;
    for (char c : s) {
      if (!c) {
        throw std::runtime_error(
            "using StrIcaseHash for a string with '\\0' character");
      }
      res = res * 311 + tolower(c);
    }
    return res;
  }
};

class StrIcaseCmp final {
 public:
  bool operator()(const std::string& l, const std::string& r) const {
    if (l.find('\0') != std::string::npos ||
        r.find('\0') != std::string::npos) {
      throw std::runtime_error(
          "using StrIcaseCmp for a string with '\\0' character");
    }
    return l.size() == r.size() && !strncasecmp(l.data(), r.data(), l.size());
  }
};

}  // namespace utils
