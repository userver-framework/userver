#include <concurrent/resettable_queue.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <tuple>
#include <vector>

#include <userver/utest/utest.hpp>
#include <userver/utils/strong_typedef.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

struct alignas(8) IntValue final {
  std::uint32_t value;
  std::uint32_t is_valid;

  static IntValue Make(std::uint32_t value) noexcept { return {value, 1}; }

  static constexpr IntValue MakeInvalid(std::uint32_t invalid_id) noexcept {
    return {invalid_id, 0};
  }

  static bool IsValid(IntValue value) noexcept { return value.is_valid != 0; }

  friend bool operator==(IntValue lhs, IntValue rhs) noexcept {
    return lhs.value == rhs.value;
  }
};

}  // namespace

TEST(ResettableQueue, Basic) {
  concurrent::impl::ResettableQueue<IntValue> queue;

  for (std::uint32_t i = 0; i < 10; ++i) {
    queue.Push(IntValue::Make(i));
  }

  for (std::uint32_t i = 0; i < 10; ++i) {
    IntValue item{};
    EXPECT_TRUE(queue.TryPop(item));
    EXPECT_EQ(item, IntValue::Make(i));
  }
}

TEST(ResettableQueue, Invalidation) {
  concurrent::impl::ResettableQueue<IntValue> queue;
  auto handle = queue.Push(IntValue::Make(5));
  queue.Remove(std::move(handle));

  IntValue item{};
  EXPECT_FALSE(queue.TryPop(item));
}

namespace {

using ProducerCount = utils::StrongTypedef<class ProducerCountTag, std::size_t>;
using ConsumerCount = utils::StrongTypedef<class ConsumerCountTag, std::size_t>;
using UseRemove = utils::StrongTypedef<class UseRemoveTag, bool>;

using StressParams = std::tuple<ProducerCount, ConsumerCount, UseRemove>;

class ResettableQueueStress : public ::testing::TestWithParam<StressParams> {};

}  // namespace

TEST_P(ResettableQueueStress, Stress) {
  const auto producer_count =
      std::get<ProducerCount>(GetParam()).GetUnderlying();
  const auto consumer_count =
      std::get<ConsumerCount>(GetParam()).GetUnderlying();
  const bool use_remove = std::get<UseRemove>(GetParam()).GetUnderlying();
  const std::uint32_t values_per_producer =
      std::numeric_limits<std::uint32_t>::max() /
      std::max(producer_count, std::size_t{1});
  constexpr std::size_t kChunkSize = 32;

  using Queue = concurrent::impl::ResettableQueue<IntValue>;
  Queue queue;
  std::atomic<bool> keep_running{true};
  std::vector<std::future<void>> tasks;

  for (std::size_t id = 0; id < producer_count; ++id) {
    tasks.push_back(std::async([&, id] {
      const std::uint32_t value_begin = id * values_per_producer;
      const std::uint32_t value_end = (id + 1) * values_per_producer;
      std::vector<Queue::ItemHandle> handles;

      for (std::uint32_t value = value_begin; value != value_end; ++value) {
        if (!keep_running) break;
        const auto item_handle = queue.Push(IntValue::Make(value));

        if (use_remove) {
          handles.push_back(item_handle);
          if (value % kChunkSize == 0) {
            for (auto& handle : handles) {
              queue.Remove(std::move(handle));
            }
            handles.clear();
          }
        }
      }
    }));
  }

  for (std::size_t id = 0; id < consumer_count; ++id) {
    tasks.push_back(std::async([&, id] {
      [[maybe_unused]] const auto unused_id = id;

      while (keep_running) {
        auto value = IntValue::Make(0);
        if (queue.TryPop(value)) {
          EXPECT_TRUE(value.is_valid);
        }
      }
    }));
  }

  std::this_thread::sleep_for(50ms);
  keep_running = false;
  for (auto& task : tasks) task.get();
}

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/, ResettableQueueStress,
    ::testing::ValuesIn(std::vector<StressParams>{
        {ProducerCount{2}, ConsumerCount{0}, UseRemove{false}},
        {ProducerCount{0}, ConsumerCount{2}, UseRemove{false}},
        {ProducerCount{1}, ConsumerCount{1}, UseRemove{false}},
        {ProducerCount{1}, ConsumerCount{2}, UseRemove{false}},
        {ProducerCount{2}, ConsumerCount{1}, UseRemove{false}},
        {ProducerCount{2}, ConsumerCount{2}, UseRemove{false}},
        {ProducerCount{2}, ConsumerCount{0}, UseRemove{true}},
        {ProducerCount{2}, ConsumerCount{2}, UseRemove{true}},
    }));

USERVER_NAMESPACE_END
