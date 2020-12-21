#include <cache/dump/common_containers.hpp>

#include <utest/utest.hpp>

#include <cache/dump/test_helpers.hpp>

namespace {

template <typename T>
void TestWriteReadEquals(const T& value) {
  EXPECT_EQ(cache::dump::WriteRead(value), value);
}

}  // namespace

TEST(CacheDumpCommonContainers, Vector) {
  TestWriteReadEquals(std::vector<int>{1, 2, 5});
  TestWriteReadEquals(std::vector<bool>{true, false});
  TestWriteReadEquals(std::vector<std::string>{});
}

TEST(CacheDumpCommonContainers, Pair) {
  TestWriteReadEquals(std::pair<int, int>{1, 2});
  TestWriteReadEquals(std::pair<const int, int>{1, 2});
}

TEST(CacheDumpCommonContainers, Map) {
  TestWriteReadEquals(std::map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadEquals(std::map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadEquals(std::map<bool, bool>{});
}

TEST(CacheDumpCommonContainers, UnorderedMap) {
  TestWriteReadEquals(std::unordered_map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadEquals(std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadEquals(std::unordered_map<bool, bool>{});
}

TEST(CacheDumpCommonContainers, Set) {
  TestWriteReadEquals(std::set<int>{1, 2, 5});
  TestWriteReadEquals(std::set<std::string>{"a", "b", "bb"});
  TestWriteReadEquals(std::set<bool>{});
}

TEST(CacheDumpCommonContainers, UnorderedSet) {
  TestWriteReadEquals(std::unordered_set<int>{1, 2, 5});
  TestWriteReadEquals(std::unordered_set<std::string>{"a", "b", "bb"});
  TestWriteReadEquals(std::unordered_set<bool>{});
}

TEST(CacheDumpCommonContainers, NestedContainers) {
  TestWriteReadEquals(
      std::vector<std::unordered_map<std::string, std::set<int>>>{
          {{"abc", {1, 3}}, {"de", {2, 4, 5}}, {"", {}}},
          {},
          {{"ghij", {10}}}});
}

TEST(CacheDumpCommonContainers, Optional) {
  TestWriteReadEquals(std::optional<int>{42});
  TestWriteReadEquals(std::optional<int>{});
  TestWriteReadEquals(std::make_optional(std::make_optional(false)));
  TestWriteReadEquals(std::make_optional(std::optional<bool>{}));
  TestWriteReadEquals(std::optional<std::optional<bool>>{});
}
