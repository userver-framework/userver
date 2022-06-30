#include <userver/utils/shared_readable_ptr.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {
utils::SharedReadablePtr<int> GetSharedPtr() {
  return std::make_shared<const int>(42);
}
}  // namespace

TEST(SharedReadablePtr, Basic) {
  using Type = utils::SharedReadablePtr<int>;
  auto guard = GetSharedPtr();
  EXPECT_TRUE(guard);

  static_assert(std::is_same_v<decltype(*guard), const int&>);
  static_assert(std::is_copy_constructible_v<Type>);
  static_assert(std::is_move_assignable_v<Type>);
  static_assert(std::is_move_constructible_v<Type>);
  static_assert(std::is_nothrow_move_assignable_v<Type>);
  static_assert(std::is_nothrow_move_constructible_v<Type>);

  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  auto guard2 = guard;
  EXPECT_EQ(*guard2, *guard);
}

TEST(SharedReadablePtr, EqualNullptr) {
  utils::SharedReadablePtr<int> null{nullptr};
  utils::SharedReadablePtr<int> data{GetSharedPtr()};

  EXPECT_EQ(null, nullptr);
  EXPECT_EQ(nullptr, null);

  EXPECT_NE(data, nullptr);
  EXPECT_NE(nullptr, data);
}

TEST(SharedReadablePtr, Make) {
  auto data1 = GetSharedPtr();
  auto data2 = utils::MakeSharedReadable<int>(42);

  EXPECT_EQ(*data1, *data2);
}

struct Pair {
  int x;
  std::string y;

  Pair(int x, std::string y) : x{x}, y{std::move(y)} {}

  bool operator==(const Pair& other) const {
    return other.x == x && other.y == y;
  }
};

TEST(SharedReadablePtr, MakePair) {
  auto data1 = std::make_shared<Pair>(1, "y");
  auto data2 = utils::MakeSharedReadable<Pair>(1, "y");

  EXPECT_EQ(*data1, *data2);
}

TEST(SharedReadablePtr, Reset) {
  auto data1 = GetSharedPtr();

  EXPECT_NE(data1, nullptr);
  data1.Reset();
  EXPECT_EQ(data1, nullptr);
}

USERVER_NAMESPACE_END
