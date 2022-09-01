#include <concurrent/resettable_queue.hpp>

#include <userver/engine/async.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct IntValue final {
  std::size_t value;

  static IntValue MakeInvalid(std::size_t invalid_id) { return {invalid_id}; }

  friend bool operator==(IntValue lhs, IntValue rhs) noexcept {
    return lhs.value == rhs.value;
  }
};

}  // namespace

TEST(ResettableQueue, Basic) {
  concurrent::impl::ResettableQueue<IntValue> queue;

  for (std::size_t i = 0; i < 10; ++i) {
    queue.Push(IntValue{queue.kInvalidStateCount + i});
  }

  for (std::size_t i = 0; i < 10; ++i) {
    IntValue item{};
    EXPECT_TRUE(queue.TryPop(item));
    EXPECT_EQ(item, IntValue{queue.kInvalidStateCount + i});
  }
}

TEST(ResettableQueue, Invalidation) {
  concurrent::impl::ResettableQueue<IntValue> queue;
  auto handle = queue.Push(IntValue{queue.kInvalidStateCount + 5});
  queue.Remove(std::move(handle));

  IntValue item{};
  EXPECT_FALSE(queue.TryPop(item));
}

USERVER_NAMESPACE_END
