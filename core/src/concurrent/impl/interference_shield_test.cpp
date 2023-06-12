#include <concurrent/impl/interference_shield.hpp>

#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace {

using ShieldedByte = concurrent::impl::InterferenceShield<char>;

struct TwoShields {
  ShieldedByte a;
  ShieldedByte b;
};

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct Interleaved {
  ShieldedByte a;
  char b{};
  char c{};
  ShieldedByte d;
  char e{};
};

constexpr std::size_t kDesiredAlignment =
    concurrent::impl::kDestructiveInterferenceSize;

}  // namespace

static_assert(sizeof(ShieldedByte) == kDesiredAlignment);
static_assert(alignof(ShieldedByte) == kDesiredAlignment);

static_assert(sizeof(TwoShields) == 2 * kDesiredAlignment);
static_assert(alignof(TwoShields) == kDesiredAlignment);
static_assert(offsetof(TwoShields, a) == 0);
static_assert(offsetof(TwoShields, b) == kDesiredAlignment);

static_assert(sizeof(Interleaved) == 4 * kDesiredAlignment);
static_assert(alignof(Interleaved) == kDesiredAlignment);
static_assert(offsetof(Interleaved, a) == 0);
static_assert(offsetof(Interleaved, b) == kDesiredAlignment);
static_assert(offsetof(Interleaved, c) == kDesiredAlignment + 1);
static_assert(offsetof(Interleaved, d) == 2 * kDesiredAlignment);
static_assert(offsetof(Interleaved, e) == 3 * kDesiredAlignment);

USERVER_NAMESPACE_END
