#include <boost/core/ignore_unused.hpp>
#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/spsc_queue.hpp>
#include <utest/utest.hpp>

TEST(SpscQueue, Ctr) {
  auto queue = engine::SpscQueue<int>::Create();
  EXPECT_EQ(0u, queue->Size());
}

TEST(SpscQueue, Consume) {
  auto queue = engine::SpscQueue<int>::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  EXPECT_TRUE(producer.Push(1));
  EXPECT_EQ(1, queue->Size());

  int value;
  EXPECT_TRUE(consumer.Pop(value));
  EXPECT_EQ(1, value);
  EXPECT_EQ(0, queue->Size());
}

TEST(SpscQueue, ConsumeMany) {
  auto queue = engine::SpscQueue<int>::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  auto constexpr N = 100;

  for (int i = 0; i < N; i++) {
    auto value = i;
    EXPECT_TRUE(producer.Push(std::move(value)));
    EXPECT_EQ(i + 1, queue->Size());
  }

  for (int i = 0; i < N; i++) {
    int value;
    EXPECT_TRUE(consumer.Pop(value));
    EXPECT_EQ(i, value);
    EXPECT_EQ(N - i - 1, queue->Size());
  }
}

TEST(SpscQueue, ProducerIsDead) {
  auto queue = engine::SpscQueue<int>::Create();
  auto consumer = queue->GetConsumer();

  int value;
  boost::ignore_unused(queue->GetProducer());
  EXPECT_FALSE(consumer.Pop(value));
}

TEST(SpscQueue, ConsumerIsDead) {
  auto queue = engine::SpscQueue<int>::Create();
  auto producer = queue->GetProducer();

  boost::ignore_unused(queue->GetConsumer());
  EXPECT_FALSE(producer.Push(0));
}

TEST(SpscQueue, Block) {
  RunInCoro([] {
    auto queue = engine::SpscQueue<int>::Create();

    auto consumer_task =
        engine::impl::Async([consumer = queue->GetConsumer()]() mutable {
          int value;
          EXPECT_TRUE(consumer.Pop(value));
          EXPECT_EQ(0, value);

          EXPECT_TRUE(consumer.Pop(value));
          EXPECT_EQ(1, value);

          EXPECT_FALSE(consumer.Pop(value));
        });

    engine::Yield();
    engine::Yield();

    {
      auto producer = queue->GetProducer();
      EXPECT_TRUE(producer.Push(0));
      engine::Yield();
      EXPECT_TRUE(producer.Push(1));
    }

    consumer_task.Get();
  });
}
