#include <userver/utils/algo.hpp>
#include <userver/utils/checked_pointer.hpp>

#include <map>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct MyStuff {
  int value;
};

using MyCheckedPtr = utils::CheckedPtr<MyStuff>;

[[maybe_unused]] MyCheckedPtr GetEmpty() { return nullptr; }

}  // namespace

TEST(CheckedPtr, ThrowOnNull) {
  // MUST NOT COMPILE
  // [[maybe_unused]] auto v = *GetEmpty();
  // [[maybe_unused]] auto v = GetEmpty().Get();
  // [[maybe_unused]] auto v = GetEmpty()->value;

  MyCheckedPtr empty = nullptr;

  EXPECT_FALSE(empty);
  EXPECT_THROW(empty.Get(), std::runtime_error);
  EXPECT_THROW(*empty, std::runtime_error);
  EXPECT_THROW([[maybe_unused]] auto v = empty->value, std::runtime_error);
}

TEST(CheckedPtr, Find) {
  std::map<std::string, int> m{{"foo", 0xf00}, {"bar", 0xba7}};
  std::unordered_map<std::string, int> um{{"foo", 0xf00}, {"bar", 0xba7}};
  const auto& cm = m;
  static_assert(std::is_same_v<decltype(utils::CheckedFind(m, "bla")),
                               utils::CheckedPtr<int>>,
                "Expect a `pointer` to a non-const value in a non-const map");
  static_assert(std::is_same_v<decltype(utils::CheckedFind(cm, "bla")),
                               utils::CheckedPtr<const int>>,
                "Expect a `pointer` to a const value in a const map");
  EXPECT_FALSE(utils::CheckedFind(m, "bla"));
  EXPECT_FALSE(utils::CheckedFind(um, "bla"));

  auto mf = utils::CheckedFind(m, "foo");
  auto umf = utils::CheckedFind(m, "bar");

  ASSERT_TRUE(mf);
  ASSERT_TRUE(umf);

  EXPECT_EQ(*mf, m["foo"]);
  EXPECT_EQ(*umf, um["bar"]);
}

TEST(CheckedPtr, PointerToPointer) {
  std::map<std::string, int*> m{{"foo", nullptr}, {"bar", nullptr}};
  const auto val1 = utils::CheckedFind(m, "bar");
  ASSERT_TRUE(val1);
  EXPECT_FALSE(*val1);

  const auto val2 = utils::CheckedFind(std::as_const(m), "foo");
  ASSERT_TRUE(val2);
  EXPECT_FALSE(*val2);

  const auto val3 = utils::CheckedFind(std::as_const(m), "nope");
  ASSERT_FALSE(val3);
}

USERVER_NAMESPACE_END
