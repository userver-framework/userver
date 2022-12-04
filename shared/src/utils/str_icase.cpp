#include <userver/utils/str_icase.hpp>

#include <algorithm>  // for std::min

#include <boost/functional/hash.hpp>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

constexpr std::size_t kUppercaseToLowerMask = 32;
static_assert((static_cast<std::size_t>('A') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('Z') | kUppercaseToLowerMask) == 'z');
static_assert((static_cast<std::size_t>('a') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('z') | kUppercaseToLowerMask) == 'z');

StrIcaseHash::StrIcaseHash()
    : seed_(std::uniform_int_distribution<std::size_t>{}(DefaultRandom())) {}

StrIcaseHash::StrIcaseHash(std::size_t seed) noexcept : seed_(seed) {}

std::size_t StrIcaseHash::operator()(std::string_view s) const& noexcept {
  // NOTE: a random seed, mixed "well enough" into string hash, should make it
  // resistant to HashDOS attacks. That is, it should make deliberate generation
  // of collisions infeasible.
  std::size_t res = seed_;
  for (const char c : s) {
    const char mask = (c >= 'A' && c <= 'Z') ? kUppercaseToLowerMask : 0;
    boost::hash_combine(res, static_cast<char>(c | mask));
  }
  return res;
}

StrCaseHash::StrCaseHash()
    : seed_(std::uniform_int_distribution<std::size_t>{}(DefaultRandom())) {}

StrCaseHash::StrCaseHash(std::size_t seed) noexcept : seed_(seed) {}

std::size_t StrCaseHash::operator()(std::string_view s) const& noexcept {
  // NOTE: a random seed, mixed "well enough" into string hash, should make it
  // resistant to HashDOS attacks. That is, it should make deliberate generation
  // of collisions infeasible.
  std::size_t res = seed_;
  for (const char c : s) {
    boost::hash_combine(res, static_cast<char>(c));
  }
  return res;
}

int StrIcaseCompareThreeWay::operator()(std::string_view lhs,
                                        std::string_view rhs) const noexcept {
  const auto min_len = std::min(lhs.size(), rhs.size());
  for (std::size_t i = 0; i < min_len; ++i) {
    unsigned char a = lhs[i];
    unsigned char b = rhs[i];

    if (a == b) continue;
    if ('A' <= a && a <= 'Z') a |= kUppercaseToLowerMask;
    if ('A' <= b && b <= 'Z') b |= kUppercaseToLowerMask;
    if (a == b) continue;

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

USERVER_NAMESPACE_END
