#include <utils/str_icase.hpp>

#include <strings.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace utils {

constexpr std::size_t kUppercaseToLowerMask = 32;
static_assert((static_cast<std::size_t>('A') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('Z') | kUppercaseToLowerMask) == 'z');
static_assert((static_cast<std::size_t>('a') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('z') | kUppercaseToLowerMask) == 'z');

std::size_t StrIcaseHash::operator()(std::string_view s) const noexcept {
  std::size_t res = 0;
  for (char c : s) {
    res = res * 311 + (static_cast<std::size_t>(c) | kUppercaseToLowerMask);
  }

  return res;
}

int StrIcaseCompareThreeWay::operator()(std::string_view lhs,
                                        std::string_view rhs) const noexcept {
  const auto min_len = std::min(lhs.size(), rhs.size());
  for (std::size_t i = 0; i < min_len; ++i) {
    const unsigned char a = lhs[i];
    const unsigned char b = rhs[i];

    const auto xored = a ^ b;
    if (!xored) {
      continue;
    }

    if (xored == kUppercaseToLowerMask) {
      if (('A' <= a && a <= 'Z') || ('A' <= b && b <= 'Z')) continue;
    }

    return static_cast<int>(a) - static_cast<int>(b);
  }

  if (lhs.size() != rhs.size()) return lhs.size() < rhs.size() ? -1 : 1;
  return 0;
}

bool StrIcaseEqual::operator()(std::string_view lhs, std::string_view rhs) const
    noexcept {
  if (lhs.size() != rhs.size()) return false;
  return StrIcaseCompareThreeWay{}(lhs, rhs) == 0;
}

bool StrIcaseLess::operator()(std::string_view lhs, std::string_view rhs) const
    noexcept {
  return StrIcaseCompareThreeWay{}(lhs, rhs) < 0;
}

}  // namespace utils
