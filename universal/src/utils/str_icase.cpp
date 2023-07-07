#include <userver/utils/str_icase.hpp>

#include <algorithm>  // for std::min

#include <userver/utils/rand.hpp>

#include <utils/impl/byte_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

constexpr std::size_t kUppercaseToLowerMask = 32;
static_assert((static_cast<std::size_t>('A') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('Z') | kUppercaseToLowerMask) == 'z');
static_assert((static_cast<std::size_t>('a') | kUppercaseToLowerMask) == 'a');
static_assert((static_cast<std::size_t>('z') | kUppercaseToLowerMask) == 'z');

StrIcaseHash::StrIcaseHash()
    : StrIcaseHash{HashSeed{std::uniform_int_distribution<std::uint64_t>{}(
                                impl::DefaultRandomForHashSeed()),
                            std::uniform_int_distribution<std::uint64_t>{}(
                                impl::DefaultRandomForHashSeed())}} {}

StrIcaseHash::StrIcaseHash(HashSeed seed) noexcept : seed_{seed} {}

std::size_t StrIcaseHash::operator()(std::string_view s) const& noexcept {
  // Out implementation of siphash returns 64bit hash and under 32 bits
  // systems it gets truncated to size_t here, which might be problematic,
  // but I'm not certain.
  // TODO : TAXICOMMON-6397, think about this
  return impl::CaseInsensitiveSipHasher{seed_.k0, seed_.k1}(s);
}

StrCaseHash::StrCaseHash()
    : StrCaseHash{HashSeed{std::uniform_int_distribution<std::uint64_t>{}(
                               impl::DefaultRandomForHashSeed()),
                           std::uniform_int_distribution<std::uint64_t>{}(
                               impl::DefaultRandomForHashSeed())}} {}

StrCaseHash::StrCaseHash(HashSeed seed) noexcept : seed_{seed} {}

std::size_t StrCaseHash::operator()(std::string_view s) const& noexcept {
  // Out implementation of siphash returns 64bit hash and under 32 bits
  // systems it gets truncated to size_t here, which might be problematic,
  // but I'm not certain.
  // TODO : TAXICOMMON-6397, think about this
  return impl::SipHasher{seed_.k0, seed_.k1}(s);
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
  return impl::CaseInsensitiveEqual{}(lhs, rhs);
}

bool StrIcaseLess::operator()(std::string_view lhs, std::string_view rhs) const
    noexcept {
  return StrIcaseCompareThreeWay{}(lhs, rhs) < 0;
}

}  // namespace utils

USERVER_NAMESPACE_END
