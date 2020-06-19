#include <gtest/gtest.h>

#include <utils/any_movable.hpp>

namespace {

const char* data =
    "Some very long string that does not fit into SSO and validates that "
    "memory usage on any_movable";

using utils::AnyMovableCast;
}  // namespace

TEST(AnyMovable, EmptyAndClear) {
  utils::AnyMovable a;
  EXPECT_TRUE(a.IsEmpty());

  a = 100;
  EXPECT_FALSE(a.IsEmpty());

  a.Clear();
  EXPECT_TRUE(a.IsEmpty());

  EXPECT_FALSE(utils::AnyMovable("hello").IsEmpty());
}

TEST(AnyMovable, CastingPointer) {
  utils::AnyMovable a{std::string(data)};
  EXPECT_FALSE(a.IsEmpty());

  EXPECT_FALSE(AnyMovableCast<int>(&a));
  EXPECT_FALSE(AnyMovableCast<char>(&a));
  EXPECT_FALSE(AnyMovableCast<const char*>(&a));
  EXPECT_TRUE(AnyMovableCast<std::string>(&a));
  EXPECT_EQ(*AnyMovableCast<std::string>(&a), data);

  a = data;
  EXPECT_FALSE(AnyMovableCast<int>(&a));
  EXPECT_FALSE(AnyMovableCast<char>(&a));
  EXPECT_TRUE(AnyMovableCast<const char*>(&a));
  EXPECT_EQ(*AnyMovableCast<const char*>(&a), data);
  EXPECT_FALSE(AnyMovableCast<std::string>(&a));
}

TEST(AnyMovable, CastingValue) {
  utils::AnyMovable a{std::string(data)};
  EXPECT_FALSE(a.IsEmpty());

  EXPECT_THROW(AnyMovableCast<int>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyMovableCast<int>(a), std::bad_any_cast);
  EXPECT_THROW(AnyMovableCast<int>(a), std::exception);

  EXPECT_NO_THROW(AnyMovableCast<std::string>(a));
  EXPECT_EQ(AnyMovableCast<std::string>(a), data);
  EXPECT_NO_THROW(AnyMovableCast<const std::string>(a));
  EXPECT_EQ(AnyMovableCast<const std::string>(a), data);
  EXPECT_NO_THROW(AnyMovableCast<const std::string&>(a));
  EXPECT_EQ(AnyMovableCast<const std::string&>(a), data);
  EXPECT_NO_THROW(AnyMovableCast<std::string&>(a));
  EXPECT_EQ(AnyMovableCast<std::string&>(a), data);

  a = data;
  EXPECT_THROW(AnyMovableCast<std::string>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyMovableCast<std::string>(a), std::bad_any_cast);
  EXPECT_THROW(AnyMovableCast<std::string>(a), std::exception);

  EXPECT_NO_THROW(AnyMovableCast<const char*>(a));
  EXPECT_EQ(AnyMovableCast<const char*>(a), data);
}

TEST(AnyMovable, CastingConstValue) {
  const utils::AnyMovable a{std::string(data)};
  EXPECT_FALSE(a.IsEmpty());

  EXPECT_THROW(AnyMovableCast<int>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyMovableCast<int>(a), std::bad_any_cast);
  EXPECT_THROW(AnyMovableCast<int>(a), std::exception);

  EXPECT_NO_THROW(AnyMovableCast<std::string>(a));
  EXPECT_EQ(AnyMovableCast<std::string>(a), data);

  EXPECT_NO_THROW(AnyMovableCast<const std::string>(a));
  EXPECT_EQ(AnyMovableCast<const std::string>(a), data);
}

TEST(AnyMovable, NonMovable) {
  using HeldType = std::unique_ptr<std::string>;
  auto value = std::make_unique<std::string>(data);
  utils::AnyMovable a{std::move(value)};
  EXPECT_FALSE(a.IsEmpty());
  EXPECT_TRUE(!value);

  EXPECT_THROW(AnyMovableCast<std::string>(a), utils::BadAnyMovableCast);
  EXPECT_THROW(AnyMovableCast<std::string>(a), std::bad_any_cast);
  EXPECT_THROW(AnyMovableCast<std::string>(a), std::exception);

  EXPECT_NO_THROW(AnyMovableCast<HeldType&>(a));
  auto extracted_value = AnyMovableCast<HeldType&&>(a);
  EXPECT_TRUE(extracted_value);
  EXPECT_EQ(*extracted_value, data);

  a = std::move(extracted_value);

  EXPECT_THROW(AnyMovableCast<std::string>(a), std::exception);
  EXPECT_NO_THROW(AnyMovableCast<HeldType&>(a));
  extracted_value = std::move(AnyMovableCast<HeldType&>(a));
  EXPECT_TRUE(extracted_value);
  EXPECT_EQ(*extracted_value, data);
}

TEST(AnyMovable, References) {
  using HeldType = std::unique_ptr<std::string>;
  auto value = std::make_unique<std::string>(data);
  utils::AnyMovable a{std::move(value)};
  EXPECT_FALSE(a.IsEmpty());
  EXPECT_TRUE(!value);

  auto* ptr = AnyMovableCast<HeldType>(&a);
  EXPECT_EQ(ptr, &AnyMovableCast<HeldType&>(a));
  EXPECT_EQ(ptr, &AnyMovableCast<const HeldType&>(a));
  EXPECT_EQ(ptr, AnyMovableCast<const HeldType>(&a));
  EXPECT_EQ(ptr, AnyMovableCast<HeldType>(&a));

  HeldType&& rval = AnyMovableCast<HeldType>(std::move(a));
  EXPECT_EQ(ptr, &rval);
  HeldType&& rval2 = AnyMovableCast<HeldType&&>(a);
  EXPECT_EQ(ptr, &rval2);
  const HeldType&& rval3 = AnyMovableCast<const HeldType&&>(a);
  EXPECT_EQ(ptr, &rval3);

  EXPECT_EQ(**ptr, data) << "data was accidentally moved out";
}

TEST(AnyMovable, MoveConstructors) {
  utils::AnyMovable a{42};
  EXPECT_FALSE(a.IsEmpty());

  utils::AnyMovable b{std::move(a)};
  EXPECT_TRUE(a.IsEmpty());
  EXPECT_FALSE(b.IsEmpty());
  EXPECT_TRUE(AnyMovableCast<int>(&b));

  a = std::move(b);
  EXPECT_TRUE(b.IsEmpty());
  EXPECT_FALSE(a.IsEmpty());
  EXPECT_TRUE(AnyMovableCast<int>(&a));

  EXPECT_FALSE((std::is_constructible<utils::AnyMovable,
                                      const utils::AnyMovable&&>::value));
  EXPECT_FALSE((std::is_constructible<utils::AnyMovable,
                                      const utils::AnyMovable&>::value));
  EXPECT_FALSE(
      (std::is_constructible<utils::AnyMovable, utils::AnyMovable&>::value));
  EXPECT_TRUE(
      (std::is_constructible<utils::AnyMovable, utils::AnyMovable&&>::value));
}
