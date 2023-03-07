#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/parse/common_containers.hpp>

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

USERVER_NAMESPACE_END
