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

  auto guard2 = guard;
  EXPECT_EQ(*guard2, *guard);
}

USERVER_NAMESPACE_END
