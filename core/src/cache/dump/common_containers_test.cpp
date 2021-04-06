#include <cache/dump/common_containers.hpp>

#include <atomic>

#include <boost/bimap.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <cache/dump/test_helpers.hpp>
#include <utest/utest.hpp>

namespace {

struct NonMovable {
  std::atomic<int> atomic{0};
};

void Write(cache::dump::Writer& writer, const NonMovable& value) {
  writer.Write(value.atomic.load());
}

NonMovable Read(cache::dump::Reader& reader, cache::dump::To<NonMovable>) {
  return NonMovable{std::atomic<int>{reader.Read<int>()}};
}

struct Dummy {
  int id;
  std::string name;
};

bool operator==(const Dummy& d1, const Dummy& d2) {
  return d1.id == d2.id && d1.name == d2.name;
}

void Write(cache::dump::Writer& writer, const Dummy& dummy) {
  writer.Write(dummy.id);
  writer.Write(dummy.name);
}

Dummy Read(cache::dump::Reader& reader, cache::dump::To<Dummy>) {
  Dummy dummy;
  dummy.id = reader.Read<int>();
  dummy.name = reader.Read<std::string>();
  return dummy;
}

}  // namespace

using cache::dump::FromBinary;
using cache::dump::TestWriteReadCycle;
using cache::dump::ToBinary;

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

TEST(CacheDumpCommonContainers, UniquePtr) {
  const auto before1 = std::unique_ptr<NonMovable>{};
  const auto after1 =
      FromBinary<std::unique_ptr<NonMovable>>(ToBinary(before1));
  EXPECT_FALSE(after1);

  const auto before2 = std::make_unique<NonMovable>();
  before2->atomic = 42;
  const auto after2 =
      FromBinary<std::unique_ptr<NonMovable>>(ToBinary(before2));
  EXPECT_EQ(before2->atomic.load(), after2->atomic.load());
}

TEST(CacheDumpCommonContainers, SharedPtr) {
  const auto before1 = std::shared_ptr<NonMovable>{};
  const auto after1 =
      FromBinary<std::shared_ptr<NonMovable>>(ToBinary(before1));
  EXPECT_FALSE(after1);

  const auto before2 = std::make_shared<NonMovable>();
  before2->atomic = 42;
  const auto after2 =
      FromBinary<std::shared_ptr<NonMovable>>(ToBinary(before2));
  EXPECT_EQ(before2->atomic.load(), after2->atomic.load());
}

TEST(CacheDumpCommonContainers, SharedPtrToSame) {
  std::pair<std::shared_ptr<int>, std::shared_ptr<int>> before;
  before.first = std::make_shared<int>(42);
  before.second = before.first;
  EXPECT_EQ(before.first.get(), before.second.get());  // same int object

  const auto after = FromBinary<decltype(before)>(ToBinary(before));
  EXPECT_NE(after.first.get(), after.second.get());  // different int objects
}

TEST(CacheDumpBoostContainers, Bimap) {
  boost::bimap<int, std::string> map;
  map.insert({1, "a"});
  map.insert({2, "b"});
  TestWriteReadCycle(map);
}

TEST(CacheDumpBoostContainers, MultiIndexWithReserve) {
  using Index =
      boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
          boost::multi_index::member<Dummy, int, &Dummy::id>>>;

  boost::multi_index_container<Dummy, Index> dummies;
  dummies.insert(Dummy{2, "b"});
  dummies.insert(Dummy{5, "a"});
  TestWriteReadCycle(dummies);
}

TEST(CacheDumpBoostContainers, MultiIndexWithoutReserve) {
  using Index =
      boost::multi_index::indexed_by<boost::multi_index::ordered_unique<
          boost::multi_index::member<Dummy, int, &Dummy::id>>>;

  boost::multi_index_container<Dummy, Index> dummies;
  dummies.insert(Dummy{2, "b"});
  dummies.insert(Dummy{5, "a"});
  TestWriteReadCycle(dummies);
}
