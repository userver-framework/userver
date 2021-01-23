#include <cache/dump/common_containers.hpp>

#include <utest/utest.hpp>

#include <cache/dump/test_helpers.hpp>

using cache::dump::TestWriteReadCycle;

TEST(CacheDumpCommonContainers, Vector) {
  TestWriteReadCycle(std::vector<int>{1, 2, 5});
  TestWriteReadCycle(std::vector<bool>{true, false});
  TestWriteReadCycle(std::vector<std::string>{});
}

TEST(CacheDumpCommonContainers, Pair) {
  TestWriteReadCycle(std::pair<int, int>{1, 2});
  TestWriteReadCycle(std::pair<const int, int>{1, 2});
}

TEST(CacheDumpCommonContainers, Map) {
  TestWriteReadCycle(std::map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadCycle(std::map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadCycle(std::map<bool, bool>{});
}

TEST(CacheDumpCommonContainers, UnorderedMap) {
  TestWriteReadCycle(std::unordered_map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadCycle(std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadCycle(std::unordered_map<bool, bool>{});
}

TEST(CacheDumpCommonContainers, Set) {
  TestWriteReadCycle(std::set<int>{1, 2, 5});
  TestWriteReadCycle(std::set<std::string>{"a", "b", "bb"});
  TestWriteReadCycle(std::set<bool>{});
}

TEST(CacheDumpCommonContainers, UnorderedSet) {
  TestWriteReadCycle(std::unordered_set<int>{1, 2, 5});
  TestWriteReadCycle(std::unordered_set<std::string>{"a", "b", "bb"});
  TestWriteReadCycle(std::unordered_set<bool>{});
}

TEST(CacheDumpCommonContainers, NestedContainers) {
  TestWriteReadCycle(
      std::vector<std::unordered_map<std::string, std::set<int>>>{
          {{"abc", {1, 3}}, {"de", {2, 4, 5}}, {"", {}}},
          {},
          {{"ghij", {10}}}});
}

TEST(CacheDumpCommonContainers, Optional) {
  TestWriteReadCycle(std::optional<int>{42});
  TestWriteReadCycle(std::optional<int>{});
  TestWriteReadCycle(std::make_optional(std::make_optional(false)));
  TestWriteReadCycle(std::make_optional(std::optional<bool>{}));
  TestWriteReadCycle(std::optional<std::optional<bool>>{});
}

TEST(CacheDumpCommonContainers, CacheDumpStrongTypedef) {
  using Loggable = utils::StrongTypedef<struct NameTag, std::string>;
  static_assert(cache::dump::kIsDumpable<Loggable>);
  TestWriteReadCycle(Loggable{"Ivan"});

  using NonLoggable = utils::NonLoggable<struct PasswordTag, std::string>;
  static_assert(cache::dump::kIsDumpable<NonLoggable>);
  TestWriteReadCycle(NonLoggable{"Ivan"});

  struct Dummy {};
  using NonDumpableContents = utils::StrongTypedef<struct DummyTag, Dummy>;
  static_assert(!cache::dump::kIsDumpable<NonDumpableContents>);
}
