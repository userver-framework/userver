#include <userver/dump/common_containers.hpp>

#include <atomic>

#include <boost/bimap.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <userver/dump/test_helpers.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct NonCopyable {
  explicit NonCopyable(int value) : value(value) {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&&) = default;

  int value;
};

bool operator==(const NonCopyable& lhs, const NonCopyable& rhs) {
  return lhs.value == rhs.value;
}

void Write(dump::Writer& writer, const NonCopyable& value) {
  writer.Write(value.value);
}

NonCopyable Read(dump::Reader& reader, dump::To<NonCopyable>) {
  return NonCopyable{reader.Read<int>()};
}

struct NonMovable {
  std::atomic<int> atomic{0};
};

void Write(dump::Writer& writer, const NonMovable& value) {
  writer.Write(value.atomic.load());
}

NonMovable Read(dump::Reader& reader, dump::To<NonMovable>) {
  return NonMovable{std::atomic<int>{reader.Read<int>()}};
}

struct Dummy {
  int id = 0;
  std::string name{};
};

bool operator==(const Dummy& d1, const Dummy& d2) {
  return d1.id == d2.id && d1.name == d2.name;
}

void Write(dump::Writer& writer, const Dummy& dummy) {
  writer.Write(dummy.id);
  writer.Write(dummy.name);
}

Dummy Read(dump::Reader& reader, dump::To<Dummy>) {
  Dummy dummy;
  dummy.id = reader.Read<int>();
  dummy.name = reader.Read<std::string>();
  return dummy;
}

}  // namespace

using dump::FromBinary;
using dump::TestWriteReadCycle;
using dump::ToBinary;

TEST(DumpCommonContainers, Vector) {
  TestWriteReadCycle(std::vector<int>{1, 2, 5});
  TestWriteReadCycle(std::vector<bool>{true, false});
  TestWriteReadCycle(std::vector<std::string>{});
}

TEST(DumpCommonContainers, Pair) {
  TestWriteReadCycle(std::pair<int, int>{1, 2});
  TestWriteReadCycle(std::pair<const int, int>{1, 2});
}

TEST(DumpCommonContainers, Map) {
  TestWriteReadCycle(std::map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadCycle(std::map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadCycle(std::map<bool, bool>{});
}

TEST(DumpCommonContainers, UnorderedMap) {
  TestWriteReadCycle(std::unordered_map<int, std::string>{{1, "a"}, {2, "b"}});
  TestWriteReadCycle(std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}});
  TestWriteReadCycle(std::unordered_map<bool, bool>{});
}

TEST(DumpCommonContainers, Set) {
  TestWriteReadCycle(std::set<int>{1, 2, 5});
  TestWriteReadCycle(std::set<std::string>{"a", "b", "bb"});
  TestWriteReadCycle(std::set<bool>{});
}

TEST(DumpCommonContainers, UnorderedSet) {
  TestWriteReadCycle(std::unordered_set<int>{1, 2, 5});
  TestWriteReadCycle(std::unordered_set<std::string>{"a", "b", "bb"});
  TestWriteReadCycle(std::unordered_set<bool>{});
}

TEST(DumpCommonContainers, NestedContainers) {
  TestWriteReadCycle(
      std::vector<std::unordered_map<std::string, std::set<int>>>{
          {{"abc", {1, 3}}, {"de", {2, 4, 5}}, {"", {}}},
          {},
          {{"ghij", {10}}}});
}

TEST(DumpCommonContainers, Optional) {
  TestWriteReadCycle(std::optional<int>{42});
  TestWriteReadCycle(std::optional<int>{});
  TestWriteReadCycle(std::make_optional(std::make_optional(false)));
  TestWriteReadCycle(std::make_optional(std::optional<bool>{}));
  TestWriteReadCycle(std::optional<std::optional<bool>>{});
}

TEST(DumpCommonContainers, Variant) {
  TestWriteReadCycle(std::variant<int>{42});
  TestWriteReadCycle(std::variant<int, std::string>{42});
  TestWriteReadCycle(std::variant<int, std::string>{"what"});
  TestWriteReadCycle(std::variant<int, int>{std::in_place_index<0>, 42});
  TestWriteReadCycle(std::variant<int, int>{std::in_place_index<1>, 42});
  TestWriteReadCycle(std::variant<NonCopyable>{NonCopyable{42}});
  static_assert(!dump::kIsDumpable<std::variant<NonMovable>>);

  UEXPECT_THROW_MSG((dump::FromBinary<std::variant<int, std::string>>("\x02")),
                    std::runtime_error, "Invalid std::variant index in dump");
  UEXPECT_THROW_MSG((dump::FromBinary<std::variant<int, std::string>>("\x42")),
                    std::runtime_error, "Invalid std::variant index in dump");
}

TEST(DumpCommonContainers, StrongTypedef) {
  using Loggable = utils::StrongTypedef<struct NameTag, std::string>;
  static_assert(dump::kIsDumpable<Loggable>);
  TestWriteReadCycle(Loggable{"Ivan"});

  using NonLoggable = utils::NonLoggable<struct PasswordTag, std::string>;
  static_assert(dump::kIsDumpable<NonLoggable>);
  TestWriteReadCycle(NonLoggable{"Ivan"});

  struct Dummy {};
  using NonDumpableContents = utils::StrongTypedef<struct DummyTag, Dummy>;
  static_assert(!dump::kIsDumpable<NonDumpableContents>);
}

TEST(DumpCommonContainers, UniquePtr) {
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

TEST(DumpCommonContainers, SharedPtr) {
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

TEST(DumpCommonContainers, SharedPtrToSame) {
  std::pair<std::shared_ptr<int>, std::shared_ptr<int>> before;
  before.first = std::make_shared<int>(42);
  before.second = before.first;
  EXPECT_EQ(before.first.get(), before.second.get());  // same int object

  const auto after = FromBinary<decltype(before)>(ToBinary(before));
  EXPECT_NE(after.first.get(), after.second.get());  // different int objects
}

TEST(DumpCommonContainers, Bimap) {
  boost::bimap<int, std::string> map;
  map.insert({1, "a"});
  map.insert({2, "b"});
  TestWriteReadCycle(map);
}

TEST(DumpCommonContainers, MultiIndexWithReserve) {
  using Index =
      boost::multi_index::indexed_by<boost::multi_index::hashed_unique<
          boost::multi_index::member<Dummy, int, &Dummy::id>>>;

  boost::multi_index_container<Dummy, Index> dummies;
  dummies.insert(Dummy{2, "b"});
  dummies.insert(Dummy{5, "a"});
  TestWriteReadCycle(dummies);
}

TEST(DumpCommonContainers, MultiIndexWithoutReserve) {
  using Index =
      boost::multi_index::indexed_by<boost::multi_index::ordered_unique<
          boost::multi_index::member<Dummy, int, &Dummy::id>>>;

  boost::multi_index_container<Dummy, Index> dummies;
  dummies.insert(Dummy{2, "b"});
  dummies.insert(Dummy{5, "a"});
  TestWriteReadCycle(dummies);
}

USERVER_NAMESPACE_END
