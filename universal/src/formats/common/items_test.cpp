#include <userver/formats/common/items.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

bool IsRvalue(std::string&&) { return true; }

}  // namespace

TEST(FormatsItems, LvalueReference) {
  auto value = formats::json::FromString(R"({"key": "value"})");
  int iterations = 0;
  for (const auto [key, value] : Items(value)) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ++iterations;
  }

  iterations = 0;
  for (auto [key, value] : Items(value)) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ASSERT_TRUE(IsRvalue(std::move(key)));
    ++iterations;
  }
}

TEST(FormatsItems, ConstLvalueReference) {
  const auto value = formats::json::FromString(R"({"key": "value"})");
  int iterations = 0;
  for (const auto& [key, value] : Items(value)) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ++iterations;
  }

  iterations = 0;
  for (auto [key, value] : Items(value)) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ASSERT_TRUE(IsRvalue(std::move(key)));
    ++iterations;
  }
}

TEST(FormatsItems, RvalueReference) {
  auto value = formats::json::FromString(R"({"key": "value"})");
  int iterations = 0;
  for (const auto& [key, value] : Items(std::move(value))) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ++iterations;
  }

  value = formats::json::FromString(R"({"key": "value"})");
  iterations = 0;
  for (auto [key, value] : Items(std::move(value))) {
    ASSERT_EQ(iterations, 0);
    ASSERT_EQ(key, "key");
    ASSERT_EQ(value.As<std::string>(), "value");

    ASSERT_TRUE(IsRvalue(std::move(key)));
  }
}

TEST(FormatsItems, Iterations) {
  auto value = formats::json::FromString(R"({"key1": "v1", "key2": "v2"})");

  auto items = Items(value);
  auto it = items.begin();

  ASSERT_NE(it, items.end());
  auto value_x = *it;
  std::string key_x = value_x.key;
  auto v_x = value_x.value;

  it++;
  ASSERT_NE(it, items.end());

  auto [key_y, v_y] = *it;

  if (key_x == "key1") {
    ASSERT_EQ(v_x.As<std::string>(), "v1");
    ASSERT_EQ(key_y, "key2");
    ASSERT_EQ(v_y.As<std::string>(), "v2");
  } else {
    ASSERT_EQ(key_x, "key2");
    ASSERT_EQ(v_x.As<std::string>(), "v2");
    ASSERT_EQ(key_y, "key1");
    ASSERT_EQ(v_y.As<std::string>(), "v1");
  }

  ++it;
  ASSERT_EQ(it, items.end());
}

TEST(FormatsItems, BoostRanges) {
  auto value = formats::json::FromString(R"({"key1": "v1", "key2": "v2"})");
  EXPECT_THAT(
      Items(value) |
          boost::adaptors::transformed(
              // Must return by value since kv is temporary and stores key.
              [](auto kv) { return std::move(kv.key); }),
      testing::UnorderedElementsAreArray({"key1", "key2"}));
  EXPECT_THAT(
      Items(value) | boost::adaptors::transformed(
                         // Can return by reference since kv only stores value&.
                         [](auto kv) -> const auto& { return kv.value; }),
      testing::UnorderedElementsAreArray(
          {formats::json::ValueBuilder{"v1"}.ExtractValue(),
           formats::json::ValueBuilder{"v2"}.ExtractValue()}));
}

USERVER_NAMESPACE_END
