#include <userver/concurrent/mpsc_queue.hpp>

#include <userver/utils/async.hpp>
#include "mp_queue_test.hpp"

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using TestTypes =
    testing::Types<concurrent::MpscQueue<int>,
                   concurrent::MpscQueue<std::unique_ptr<int>>,
                   concurrent::MpscQueue<std::unique_ptr<RefCountData>>>;

constexpr std::size_t kProducersCount = 4;
constexpr std::size_t kMessageCount = 1000;
}  // namespace

INSTANTIATE_TYPED_UTEST_SUITE_P(MpscQueue, QueueFixture,
                                concurrent::MpscQueue<int>);

INSTANTIATE_TYPED_UTEST_SUITE_P(MpscQueue, TypedQueueFixture, TestTypes);

UTEST(MpscQueue, ConsumerIsDead) {
  auto queue = concurrent::MpscQueue<int>::Create();
  auto producer = queue->GetProducer();

  (void)(queue->GetConsumer());
  EXPECT_FALSE(producer.Push(0));
}

UTEST(MpscQueue, SampleMpscQueue) {
  /// [Sample concurrent::MpscQueue usage]
  static constexpr std::chrono::milliseconds kTimeout{10};

  auto queue = concurrent::MpscQueue<int>::Create();
  auto producer = queue->GetProducer();
  auto consumer = queue->GetConsumer();

  auto producer_task = utils::Async("producer", [&] {
    // ...
    if (!producer.Push(1, engine::Deadline::FromDuration(kTimeout))) {
      // The reader is dead
    }
  });

  auto consumer_task = utils::Async("consumer", [&] {
    for (;;) {
      // ...
      int item{};
      if (consumer.Pop(item, engine::Deadline::FromDuration(kTimeout))) {
        // processing the queue element
        ASSERT_EQ(item, 1);
      } else {
        // the queue is empty and there are no more live producers
        return;
      }
    }
  });
  producer_task.Get();
  consumer_task.Get();
  /// [Sample concurrent::MpscQueue usage]
}

UTEST_MT(MpscQueue, MultiProducer, 3) {
  auto queue = concurrent::MpscQueue<int>::Create();
  auto producer_1 = queue->GetProducer();
  auto producer_2 = queue->GetProducer();
  auto consumer = queue->GetConsumer();

  queue->SetSoftMaxSize(2);
  ASSERT_TRUE(producer_1.PushNoblock(1));
  ASSERT_TRUE(producer_2.PushNoblock(2));
  auto task1 = utils::Async("pusher", [&] { ASSERT_TRUE(producer_1.Push(3)); });
  auto task2 = utils::Async("pusher", [&] { ASSERT_TRUE(producer_2.Push(4)); });
  ASSERT_FALSE(task1.IsFinished());
  ASSERT_FALSE(task2.IsFinished());
  EXPECT_EQ(queue->GetSizeApproximate(), 2);

  int value_1{0};
  int value_2{0};
  ASSERT_TRUE(consumer.PopNoblock(value_1));
  ASSERT_TRUE(consumer.PopNoblock(value_2));
  ASSERT_EQ(value_1, 1);
  ASSERT_EQ(value_2, 2);

  ASSERT_TRUE(consumer.Pop(value_1));
  ASSERT_TRUE(consumer.Pop(value_2));
  // Don't know who (task1 or task2) woke up first.
  ASSERT_TRUE((value_1 == 3 && value_2 == 4) || (value_1 == 4 && value_2 == 3));

  task1.Get();
  task2.Get();

  EXPECT_EQ(queue->GetSizeApproximate(), 0);
}

UTEST_MT(MpscQueue, FifoTest, kProducersCount + 1) {
  auto queue = concurrent::MpscQueue<std::size_t>::Create(kMessageCount);
  std::vector<concurrent::MpscQueue<std::size_t>::Producer> producers;
  producers.reserve(kProducersCount);
  for (std::size_t i = 0; i < kProducersCount; ++i) {
    producers.emplace_back(queue->GetProducer());
  }

  auto consumer = queue->GetConsumer();

  std::vector<engine::TaskWithResult<void>> producers_tasks;
  producers_tasks.reserve(kProducersCount);
  for (std::size_t i = 0; i < kProducersCount; ++i) {
    producers_tasks.push_back(
        utils::Async("producer", [&producer = producers[i], i] {
          for (std::size_t message = i * kMessageCount;
               message < (i + 1) * kMessageCount; ++message) {
            ASSERT_TRUE(producer.Push(std::size_t{message}));
          }
        }));
  }

  std::vector<std::size_t> consumed_messages(kMessageCount * kProducersCount,
                                             0);
  auto consumer_task = utils::Async("consumer", [&] {
    std::vector<std::size_t> previous(kProducersCount, 0);

    std::size_t value{};
    while (consumer.Pop(value)) {
      std::size_t step = value / kMessageCount;
      ASSERT_TRUE(previous[step] == 0 ||
                  previous[step] < value % kMessageCount);
      previous[step] = value % kMessageCount;
      ++consumed_messages[value];
    }
  });

  for (auto& task : producers_tasks) {
    task.Get();
  }
  producers.clear();

  consumer_task.Get();

  ASSERT_TRUE(std::all_of(consumed_messages.begin(), consumed_messages.end(),
                          [](int item) { return (item == 1); }));
}

USERVER_NAMESPACE_END
