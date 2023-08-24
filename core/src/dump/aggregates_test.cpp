#include <userver/dump/aggregates.hpp>

#include <atomic>
#include <memory>

#include <userver/dump/common_containers.hpp>
#include <userver/dump/test_helpers.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct Empty {};

bool operator==(Empty, Empty) { return true; }

struct NonMovable {
  NonMovable() = default;
  NonMovable(NonMovable&&) = delete;
};

[[maybe_unused]] void Write(dump::Writer&, const NonMovable&) {}

[[maybe_unused]] NonMovable Read(dump::Reader&, dump::To<NonMovable>) {
  return {};
}

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

void Write(dump::Writer& writer, const NonAggregate& na) {
  writer.Write(na.GetFoo());
}

NonAggregate Read(dump::Reader& reader, dump::To<NonAggregate>) {
  return NonAggregate{reader.Read<int>()};
}

}  // namespace

template <>
struct dump::IsDumpedAggregate<Empty>;

template <>
struct dump::IsDumpedAggregate<MyPair>;

template <typename T>
struct dump::IsDumpedAggregate<Single<T>>;

using dump::TestWriteReadCycle;

TEST(DumpAggregates, Empty) { TestWriteReadCycle(Empty{}); }

TEST(DumpAggregates, OwnPair) { TestWriteReadCycle(MyPair{42, "abc"}); }

TEST(DumpAggregates, Nested) {
  TestWriteReadCycle(Single<NonAggregate>{NonAggregate{42}});
  TestWriteReadCycle(Single<Single<Single<int>>>{{{42}}});
}

static_assert(dump::kIsDumpable<Single<int>>);
static_assert(dump::kIsDumpable<Single<NonAggregate>>);
static_assert(dump::kIsDumpable<Single<std::unique_ptr<int>>>);
static_assert(dump::kIsDumpable<NonMovable>);

#if __GNUC__ >= 8 || defined(__clang__)
static_assert(dump::kIsDumpable<Single<NonMovable>>);
#endif

USERVER_NAMESPACE_END
