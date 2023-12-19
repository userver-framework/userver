#include <gtest/gtest.h>

#include <chrono>
#include <unordered_map>
#include <unordered_set>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Hash {
  size_t operator()(int a) const { return a - 20; }
};

struct StrHash {
  size_t operator()(const std::string& a) const {
    return std::hash<std::string>{}(a);
  }
};

struct KeyEqual {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return lhs == rhs;
  }
};

}  // namespace

TEST(FormatsJson, HashCompile) {
  EXPECT_EQ(
      (std::unordered_set<int>{1, 2, 3}),
      (formats::json::FromString("[1,2,3]").As<std::unordered_set<int>>()));
  EXPECT_EQ((std::unordered_set<int, Hash>{1, 2, 3}),
            (formats::json::FromString("[1,2,3]")
                 .As<std::unordered_set<int, Hash>>()));

  EXPECT_EQ((std::unordered_map<std::string, int>{{"1", 2}}),
            (formats::json::FromString("{\"1\": 2}")
                 .As<std::unordered_map<std::string, int>>()));
  EXPECT_EQ((std::unordered_map<std::string, int, StrHash>{{"1", 2}}),
            (formats::json::FromString("{\"1\": 2}")
                 .As<std::unordered_map<std::string, int, StrHash>>()));
  EXPECT_EQ(
      (std::unordered_map<std::string, int, StrHash, KeyEqual>{{"1", 2}}),
      (formats::json::FromString("{\"1\": 2}")
           .As<std::unordered_map<std::string, int, StrHash, KeyEqual>>()));
}

TEST(FormatsJson, ParseMilliseconds) {
  EXPECT_EQ(formats::json::FromString("42").As<std::chrono::milliseconds>(),
            std::chrono::milliseconds{42});
  UEXPECT_THROW(
      formats::json::FromString(R"( "42ms" )").As<std::chrono::milliseconds>(),
      formats::json::Exception);
}

TEST(FormatsJson, ParseSeconds) {
  EXPECT_EQ(formats::json::FromString("42").As<std::chrono::seconds>(),
            std::chrono::seconds{42});
  // Legacy: JSON should not support parsing duration from string, but it
  // currently does. What follows is essentially a golden test.
  EXPECT_EQ(formats::json::FromString(R"( "42s" )").As<std::chrono::seconds>(),
            std::chrono::seconds{42});
}

TEST(FormatsJson, ParseMinutes) {
  EXPECT_EQ(formats::json::FromString("42").As<std::chrono::minutes>(),
            std::chrono::minutes{42});
  UEXPECT_THROW(
      formats::json::FromString(R"( "42m" )").As<std::chrono::minutes>(),
      formats::json::Exception);
}

TEST(FormatsJson, ParseHours) {
  EXPECT_EQ(formats::json::FromString("42").As<std::chrono::hours>(),
            std::chrono::hours{42});
  UEXPECT_THROW(
      formats::json::FromString(R"( "42h" )").As<std::chrono::hours>(),
      formats::json::Exception);
}

USERVER_NAMESPACE_END
