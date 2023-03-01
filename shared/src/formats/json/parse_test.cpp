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
}

USERVER_NAMESPACE_END
