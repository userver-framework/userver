#include <utils/str_icase.hpp>

#include <strings.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace utils {

std::size_t StrIcaseHash::operator()(const std::string& s) const {
  std::size_t res = 0;
  for (char c : s) {
    if (!c) {
      throw std::runtime_error(
          "using StrIcaseHash for a string with '\\0' character");
    }
    res = res * 311 + std::tolower(c);
  }
  return res;
}

int StrIcaseCompareThreeWay::operator()(utils::string_view lhs,
                                        utils::string_view rhs) const {
  if (lhs.find('\0') != utils::string_view::npos ||
      rhs.find('\0') != utils::string_view::npos) {
    throw std::runtime_error(
        "cannot case-insensitively compare strings with '\\0' character");
  }

  const int cmp_result =
      ::strncasecmp(lhs.data(), rhs.data(), std::min(lhs.size(), rhs.size()));
  if (cmp_result == 0) {
    if (lhs.size() < rhs.size()) return -1;
    if (lhs.size() > rhs.size()) return 1;
    return 0;
  }
  return cmp_result;
}

bool StrIcaseEqual::operator()(utils::string_view lhs,
                               utils::string_view rhs) const {
  if (lhs.size() != rhs.size()) return false;
  return icase_cmp_(lhs, rhs) == 0;
}

bool StrIcaseLess::operator()(utils::string_view lhs,
                              utils::string_view rhs) const {
  return icase_cmp_(lhs, rhs) < 0;
}

}  // namespace utils
