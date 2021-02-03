#include <cache/dump/aggregates.hpp>

#include <atomic>
#include <memory>

#include <cache/dump/common_containers.hpp>
#include <cache/dump/test_helpers.hpp>
#include <utest/utest.hpp>

namespace {

struct Empty {};

bool operator==(Empty, Empty) { return true; }

struct MyPair {
  int x;
  std::string y;
};

bool operator==(const MyPair& lhs, const MyPair& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
struct Single {
  T member;
};

template <typename T>
bool operator==(const Single<T>& lhs, const Single<T>& rhs) {
  return lhs.member == rhs.member;
}

class NonAggregate {
 public:
  explicit NonAggregate(int foo) : foo_(foo) {}

  int GetFoo() const { return foo_; }

 private:
  int foo_;
};

bool operator==(const NonAggregate& lhs, const NonAggregate& rhs) {
  return lhs.GetFoo() == rhs.GetFoo();
}

void Write(cache::dump::Writer& writer, const NonAggregate& na) {
  writer.Write(na.GetFoo());
}

NonAggregate Read(cache::dump::Reader& reader, cache::dump::To<NonAggregate>) {
  return NonAggregate{reader.Read<int>()};
}

static_assert(cache::dump::kIsDumpable<Single<NonAggregate>>);

}  // namespace

using cache::dump::TestWriteReadCycle;

TEST(CacheDumpAggregates, Empty) { TestWriteReadCycle(Empty{}); }

TEST(CacheDumpAggregates, OwnPair) { TestWriteReadCycle(MyPair{42, "abc"}); }

TEST(CacheDumpAggregates, Nested) {
  TestWriteReadCycle(Single<NonAggregate>{NonAggregate{42}});
  TestWriteReadCycle(Single<Single<Single<int>>>{{{42}}});
}

TEST(CacheDumpAggregates, Traits) {
  EXPECT_TRUE(cache::dump::kIsDumpable<Single<int>>);
  EXPECT_TRUE(cache::dump::kIsDumpable<Single<std::unique_ptr<int>>>);
  EXPECT_FALSE(cache::dump::kIsDumpable<Single<std::atomic<int>>>);
}
