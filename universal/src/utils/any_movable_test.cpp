#include <gtest/gtest.h>

#include <userver/utils/any_movable.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const char* const kData =
    "Some very long string that does not fit into SSO and validates that "
    "memory usage on any_movable";

// for muting clang-tidy use-after-move detection
void StillAlive(utils::AnyMovable&) {}

}  // namespace

using utils::AnyCast;

TEST(AnyMovable, HasValueAndReset) {
  utils::AnyMovable a;
  EXPECT_FALSE(a.HasValue());

  a = 100;
  EXPECT_TRUE(a.HasValue());

  a.Reset();
  EXPECT_FALSE(a.HasValue());

  EXPECT_TRUE(utils::AnyMovable("hello").HasValue());
}

TEST(AnyMovable, Snippet) {
  /// [AnyMovable example usage]
  utils::AnyMovable a{std::string("Hello")};
  EXPECT_EQ(utils::AnyCast<std::string&>(a), "Hello");
  EXPECT_THROW(utils::AnyCast<int>(a), utils::BadAnyMovableCast);
  /// [AnyMovable example usage]
}

TEST(AnyMovable, CastingPointer) {
  utils::AnyMovable a{std::string(kData)};

  EXPECT_FALSE(AnyCast<int>(&a));
  EXPECT_FALSE(AnyCast<char>(&a));
  EXPECT_FALSE(AnyCast<const char*>(&a));
  EXPECT_TRUE(AnyCast<std::string>(&a));
  EXPECT_EQ(*AnyCast<std::string>(&a), kData);

  a = kData;
  EXPECT_FALSE(AnyCast<int>(&a));
  EXPECT_FALSE(AnyCast<char>(&a));
  EXPECT_TRUE(AnyCast<const char*>(&a));
  EXPECT_EQ(*AnyCast<const char*>(&a), kData);
  EXPECT_FALSE(AnyCast<std::string>(&a));
}

TEST(AnyMovable, CastingValue) {
  utils::AnyMovable a{std::string(kData)};

  EXPECT_THROW(AnyCast<int>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyCast<int>(a), std::bad_any_cast);
  EXPECT_THROW(AnyCast<int>(a), std::exception);

  EXPECT_NO_THROW(AnyCast<std::string>(a));
  EXPECT_EQ(AnyCast<std::string>(a), kData);
  EXPECT_NO_THROW(AnyCast<const std::string>(a));
  EXPECT_EQ(AnyCast<const std::string>(a), kData);
  EXPECT_NO_THROW(AnyCast<const std::string&>(a));
  EXPECT_EQ(AnyCast<const std::string&>(a), kData);
  EXPECT_NO_THROW(AnyCast<std::string&>(a));
  EXPECT_EQ(AnyCast<std::string&>(a), kData);

  a = kData;
  EXPECT_THROW(AnyCast<std::string>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyCast<std::string>(a), std::bad_any_cast);
  EXPECT_THROW(AnyCast<std::string>(a), std::exception);

  EXPECT_NO_THROW(AnyCast<const char*>(a));
  EXPECT_EQ(AnyCast<const char*>(a), kData);
}

TEST(AnyMovable, CastingConstValue) {
  const utils::AnyMovable a{std::string(kData)};

  EXPECT_THROW(AnyCast<int>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyCast<int>(a), std::bad_any_cast);
  EXPECT_THROW(AnyCast<int>(a), std::exception);

  EXPECT_NO_THROW(AnyCast<std::string>(a));
  EXPECT_EQ(AnyCast<std::string>(a), kData);

  EXPECT_NO_THROW(AnyCast<const std::string>(a));
  EXPECT_EQ(AnyCast<const std::string>(a), kData);
}

TEST(AnyMovable, NonCopyable) {
  using HeldType = std::unique_ptr<std::string>;
  auto value = std::make_unique<std::string>(kData);
  utils::AnyMovable a{std::move(value)};
  EXPECT_FALSE(value);

  EXPECT_THROW(AnyCast<std::string>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyCast<std::string>(a), std::bad_any_cast);
  EXPECT_THROW(AnyCast<std::string>(a), std::exception);

  EXPECT_NO_THROW(AnyCast<HeldType&>(a));
  auto extracted_value = AnyCast<HeldType&&>(a);
  EXPECT_TRUE(extracted_value);
  EXPECT_EQ(*extracted_value, kData);

  a = std::move(extracted_value);

  EXPECT_THROW(AnyCast<std::string>(a), std::exception);
  EXPECT_NO_THROW(AnyCast<HeldType&>(a));
  extracted_value = std::move(AnyCast<HeldType&>(a));
  EXPECT_TRUE(extracted_value);
  EXPECT_EQ(*extracted_value, kData);
}

TEST(AnyMovable, References) {
  using HeldType = std::unique_ptr<const std::string>;
  auto value = std::make_unique<const std::string>(kData);
  utils::AnyMovable a{std::move(value)};
  EXPECT_FALSE(value);

  auto* ptr = AnyCast<HeldType>(&a);
  EXPECT_EQ(ptr, AnyCast<HeldType>(&a));
  EXPECT_EQ(ptr, AnyCast<HeldType>(&std::as_const(a)));

  EXPECT_EQ(ptr, &AnyCast<HeldType&>(a));
  EXPECT_EQ(ptr, &AnyCast<const HeldType&>(a));
}

TEST(AnyMovable, RvalueReferences) {
  using HeldType = std::unique_ptr<const std::string>;
  utils::AnyMovable a{std::make_unique<const std::string>(kData)};
  auto* ptr = AnyCast<HeldType>(&a);

  decltype(auto) rvalue1 = AnyCast<HeldType&&>(a);
  StillAlive(a);
  static_assert(std::is_same_v<decltype(rvalue1), HeldType&&>);
  EXPECT_EQ(ptr, &rvalue1);
  ASSERT_EQ(**ptr, kData) << "data was accidentally moved out";

  // decltype(auto) illegal1 = AnyCast<HeldType&&>(std::as_const(a));

  // decltype(auto) illegal2 = AnyCast<HeldType&>(move(a));

  decltype(auto) rvalue2 = AnyCast<const HeldType&>(std::move(a));
  StillAlive(a);
  static_assert(std::is_same_v<decltype(rvalue2), const HeldType&>);
  EXPECT_EQ(ptr, &rvalue2);
  ASSERT_EQ(**ptr, kData) << "data was accidentally moved out";

  decltype(auto) rvalue3 = AnyCast<HeldType&&>(std::move(a));
  StillAlive(a);
  static_assert(std::is_same_v<decltype(rvalue3), HeldType&&>);
  EXPECT_EQ(ptr, &rvalue3);
  ASSERT_EQ(**ptr, kData) << "data was accidentally moved out";

  decltype(auto) rvalue4 = AnyCast<HeldType>(std::move(a));
  StillAlive(a);
  static_assert(std::is_same_v<decltype(rvalue4), HeldType>);
  EXPECT_NE(ptr, &rvalue4);
  ASSERT_FALSE(*ptr) << "data should be moved out";
  EXPECT_TRUE(rvalue4);
  EXPECT_TRUE(a.HasValue()) << "'a' should still contain a moved-from HeldType";
}

TEST(AnyMovable, MoveConstructors) {
  utils::AnyMovable a{42};
  EXPECT_TRUE(a.HasValue());

  utils::AnyMovable b{std::move(a)};
  EXPECT_TRUE(b.HasValue());
  EXPECT_TRUE(AnyCast<int>(&b));

  a = std::move(b);
  EXPECT_TRUE(a.HasValue());
  EXPECT_TRUE(AnyCast<int>(&a));

  EXPECT_FALSE((std::is_constructible<utils::AnyMovable,
                                      const utils::AnyMovable&&>::value));
  EXPECT_FALSE((std::is_constructible<utils::AnyMovable,
                                      const utils::AnyMovable&>::value));
  EXPECT_FALSE(
      (std::is_constructible<utils::AnyMovable, utils::AnyMovable&>::value));
  EXPECT_TRUE(
      (std::is_constructible<utils::AnyMovable, utils::AnyMovable&&>::value));
}

TEST(AnyMovable, InPlace) {
  struct NonMovable final {
    explicit NonMovable(int a, int b, int c) : value(a + b + c) {}

    NonMovable& operator=(NonMovable&&) = delete;

    int value;
  };

  utils::AnyMovable a{std::in_place_type<int>, 42};
  EXPECT_EQ(AnyCast<int&>(a), 42);

  utils::AnyMovable b{std::in_place_type<std::string>, kData};
  EXPECT_EQ(AnyCast<std::string&>(b), kData);

  utils::AnyMovable c{std::in_place_type<std::string>, kData, 4};
  EXPECT_EQ(AnyCast<std::string&>(c), "Some");

  utils::AnyMovable d{std::in_place_type<std::unique_ptr<int>>, new int{42}};
  EXPECT_EQ(*AnyCast<std::unique_ptr<int>&>(d), 42);

  utils::AnyMovable e{std::in_place_type<NonMovable>, 1, 2, 3};
  EXPECT_EQ(AnyCast<NonMovable&>(e).value, 6);

  a.Emplace<std::string>(kData);
  EXPECT_EQ(AnyCast<std::string&>(a), kData);
  a.Emplace<NonMovable>(1, 2, 3);
  EXPECT_EQ(AnyCast<NonMovable&>(a).value, 6);
}

USERVER_NAMESPACE_END
