#include <userver/utils/lazy_prvalue.hpp>

#include <atomic>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

/// [LazyPrvalue sample]
// Suppose that 'NonMovable' comes from an external library.
class NonMovable final {
 public:
  // The only proper way to obtain a 'NonMovable' is through 'Make' factory
  // function.
  static NonMovable Make() { return NonMovable(42); }

  NonMovable(NonMovable&&) = delete;
  NonMovable& operator=(NonMovable&&) = delete;

  int Get() const { return value_; }

 private:
  explicit NonMovable(int value) : value_(value) {}

  int value_;
};

TEST(LazyPrvalue, Optional) {
  std::optional<NonMovable> opt;

  // If 'std::optional', in addition to 'emplace' method, had 'emplace_func',
  // then we could use it to in-place construct 'NonMovable' there. Alas,
  // 'std::optional' has no such method. But we can obtain its functionality
  // using 'utils::LazyPrvalue'.
  opt.emplace(utils::LazyPrvalue([] { return NonMovable::Make(); }));

  EXPECT_EQ(opt->Get(), 42);
}
/// [LazyPrvalue sample]

int IntIdentity(int x) { return x; }

TEST(LazyPrvalue, ImplicitlyConvertible) {
  auto f = [] { return 42; };
  EXPECT_EQ(IntIdentity(utils::LazyPrvalue(f)), 42);
}

auto IntPtrIdentity(std::unique_ptr<int>&& x) { return std::move(x); }

TEST(LazyPrvalue, Mutable) {
  const auto ptr = IntPtrIdentity(utils::LazyPrvalue(
      [x = std::make_unique<int>(42)]() mutable { return std::move(x); }));
  EXPECT_EQ(*ptr, 42);
}

TEST(LazyPrvalue, MakeShared) {
  const auto ptr = std::make_shared<NonMovable>(
      utils::LazyPrvalue([] { return NonMovable::Make(); }));
  EXPECT_EQ(ptr->Get(), 42);
}

TEST(LazyPrvalue, Tuple) {
  const auto tup = std::tuple<int, std::atomic<int>>(
      5, utils::LazyPrvalue([] { return std::atomic<int>(10); }));

  const auto& [x, y] = tup;
  EXPECT_EQ(x, 5);
  EXPECT_EQ(y, 10);
}

TEST(LazyPrvalue, Constexpr) {
  constexpr std::optional<std::atomic<int>> opt =
      utils::LazyPrvalue([] { return std::atomic<int>(42); });
  EXPECT_EQ(*opt, 42);
}

}  // namespace

USERVER_NAMESPACE_END
