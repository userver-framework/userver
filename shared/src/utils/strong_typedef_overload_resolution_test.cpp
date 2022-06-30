#include <userver/utils/strong_typedef.hpp>

#include <string>

#include <gtest/gtest.h>

// Tests in this file make sure that the overloads are found (via ADL and other
// means).

namespace {

namespace some_namespace {

using MyString =
    USERVER_NAMESPACE::utils::StrongTypedef<class MyStringTag, std::string>;
struct MyString2 final
    : USERVER_NAMESPACE::utils::StrongTypedef<MyString2, std::string> {
  using StrongTypedef::StrongTypedef;
};

bool OverloadResolutionRValue(MyString&&) { return true; }
bool OverloadResolutionRValue(MyString2&&) { return false; }
bool OverloadResolutionRValue(std::string&&) { return false; }

bool OverloadResolutionLValue(const MyString&) { return true; }
bool OverloadResolutionLValue(const MyString2&) { return false; }
bool OverloadResolutionLValue(const std::string&) { return false; }

using MyId = USERVER_NAMESPACE::utils::StrongTypedef<class MyIdTag, int>;

// Comparison operators for StrongTypedef.Id* tests
template <class T>
constexpr bool operator==(MyId, T) {
  return true;
}

template <class T>
constexpr bool operator==(T, MyId) {
  return true;
}

}  // namespace some_namespace

}  // namespace

USERVER_NAMESPACE_BEGIN

TEST(StrongTypedef, StringOverloadResolutionADL) {
  some_namespace::MyString str{"word"};

  EXPECT_TRUE(OverloadResolutionLValue(some_namespace::MyString{"word"}));
  EXPECT_TRUE(OverloadResolutionLValue(str));
  EXPECT_FALSE(OverloadResolutionLValue(some_namespace::MyString2{"word"}));

  EXPECT_TRUE(OverloadResolutionRValue(some_namespace::MyString{"word"}));
  EXPECT_FALSE(OverloadResolutionRValue(some_namespace::MyString2{"word"}));
}

TEST(StrongTypedef, StringOverloadResolution) {
  // NOLINTNEXTLINE(google-build-using-namespace)
  using namespace some_namespace;
  MyString str{"word"};

  EXPECT_TRUE(OverloadResolutionLValue(MyString{"word"}));
  EXPECT_TRUE(OverloadResolutionLValue(str));
  EXPECT_FALSE(OverloadResolutionLValue(MyString2{"word"}));
  EXPECT_FALSE(OverloadResolutionLValue(str.GetUnderlying()));

  EXPECT_TRUE(OverloadResolutionRValue(MyString{"word"}));
  EXPECT_FALSE(OverloadResolutionRValue(MyString2{"word"}));
  EXPECT_FALSE(OverloadResolutionRValue(MyString{"word"}.GetUnderlying()));
}

TEST(StrongTypedef, IdADL) {
  some_namespace::MyId id{1};

  // Following lines will fail to compile because of ambiguity if there are
  // transparent comparison operators from StrongTypedef.
  EXPECT_EQ(id, 1);
  EXPECT_EQ(1, id);
}

TEST(StrongTypedef, Id) {
  // NOLINTNEXTLINE(google-build-using-namespace)
  using namespace some_namespace;
  MyId id{1};

  // Following lines will fail to compile because of ambiguity if there are
  // transparent comparison operators from StrongTypedef.
  EXPECT_EQ(id, 1);
  EXPECT_EQ(1, id);
}

USERVER_NAMESPACE_END
