#include <gtest/gtest-typed-test.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

template <class Builder>
struct Roundtrip : public ::testing::Test {
  template <typename T>
  void Check(const T& raw_const) {
    // check all ref-cases as we may use different serializers/accessors
    EXPECT_EQ(raw_const, DoRoundtrip(raw_const));
    auto raw_copy = raw_const;
    EXPECT_EQ(raw_const, DoRoundtrip(raw_copy));
    EXPECT_EQ(raw_const, DoRoundtrip(std::move(raw_copy)));
  }

 private:
  template <typename T>
  auto DoRoundtrip(T&& raw) {
    return Builder{std::forward<T>(raw)}
        .ExtractValue()
        .template As<std::decay_t<T>>();
  }
};
TYPED_TEST_SUITE_P(Roundtrip);

TYPED_TEST_P(Roundtrip, Bool) { this->Check(true); }

TYPED_TEST_P(Roundtrip, Int) { this->Check(1); }

TYPED_TEST_P(Roundtrip, Uint64) { this->Check(uint64_t{1}); }

TYPED_TEST_P(Roundtrip, Double) { this->Check(1.0); }

TYPED_TEST_P(Roundtrip, Cstring) {
  // cannot do As<char[]>(), test it manually
  constexpr const char* kTest = "test";
  const auto value = TypeParam{kTest}.ExtractValue();
  EXPECT_EQ(kTest, value.template As<std::string>());
}

TYPED_TEST_P(Roundtrip, String) { this->Check(std::string{"test"}); }

TYPED_TEST_P(Roundtrip, Optional) {
  this->Check(std::optional<int>{});
  this->Check(std::optional<int>{1});
}

TYPED_TEST_P(Roundtrip, Vector) {
  this->Check(std::vector<int>{1, 2, 3});
  this->Check(std::vector<bool>{true, false});
}

TYPED_TEST_P(Roundtrip, UnorderedMap) {
  this->Check(std::unordered_map<std::string, int>{{"one", 2}, {"three", 4}});
}

TYPED_TEST_P(Roundtrip, UnorderedSet) {
  this->Check(std::unordered_set<int>{1, 2, 3});
}

TYPED_TEST_P(Roundtrip, VectorOfStringView) {
  constexpr std::string_view kValue1 = "Some string in std::string_view 1";
  constexpr std::string_view kValue2 = "Some string in std::string_view 2";
  std::vector<std::string_view> data = {kValue1, kValue2};

  // cannot do As<std::vector<std::string_view>>(), test it manually
  const auto extracted =
      TypeParam{data}.ExtractValue().template As<std::vector<std::string>>();
  ASSERT_EQ(data.size(), extracted.size());
  ASSERT_EQ(data[0], extracted[0]);
  ASSERT_EQ(data[1], extracted[1]);
}

REGISTER_TYPED_TEST_SUITE_P(Roundtrip,

                            Bool, Int, Uint64, Double, Cstring, String,
                            Optional, Vector, UnorderedMap, UnorderedSet,
                            VectorOfStringView);

USERVER_NAMESPACE_END
