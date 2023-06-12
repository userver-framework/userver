#include <userver/utest/utest.hpp>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

// This class tracks the number of created and destroyed objects
struct RefCountData final {
  int val{0};

  // signed to allow for erroneous deletion to be detected.
  static inline std::atomic<std::int64_t> objects_count{0};

  RefCountData(int value = 0) : val(value) { objects_count.fetch_add(1); }
  ~RefCountData() { objects_count.fetch_sub(1); }
  // Non-copyable, non-movable
  RefCountData(const RefCountData&) = delete;
  void operator=(const RefCountData&) = delete;
  RefCountData(RefCountData&&) = delete;
  void operator=(RefCountData&&) = delete;
};

/// ValueHelper and its overloads provide methods:
/// 'Wrap', that accepts and int tag and must return an object of type T.
/// 'Unwrap', that extracts tag back. Unwrap(Wrap(X)) == X
template <typename T>
struct ValueHelper {};

/// Overload for int
template <>
struct ValueHelper<int> {
  static int Wrap(int tag) { return tag; }
  static int Unwrap(int tag) { return tag; }
  static bool HasMemoryLeakCheck() { return false; }
  static bool CheckMemoryOk() { return true; }
  static bool CheckWasNotMovedOut(int) { return true; }
};

template <>
struct ValueHelper<std::unique_ptr<int>> {
  static auto Wrap(int tag) { return std::make_unique<int>(tag); }
  static auto Unwrap(const std::unique_ptr<int>& ptr) { return *ptr; }
  static bool HasMemoryLeakCheck() { return false; }
  static bool CheckMemoryOk() { return true; }
  static bool CheckWasNotMovedOut(const std::unique_ptr<int>& ptr) {
    return !!ptr;
  }
};

template <>
struct ValueHelper<std::unique_ptr<RefCountData>> {
  static auto Wrap(int tag) { return std::make_unique<RefCountData>(tag); }
  static auto Unwrap(const std::unique_ptr<RefCountData>& data) {
    return data->val;
  }
  // Checks that no object was leaked
  static bool CheckMemoryOk() {
    return RefCountData::objects_count.load() == 0;
  }
  static bool HasMemoryLeakCheck() { return true; }
  static bool CheckWasNotMovedOut(const std::unique_ptr<RefCountData>& ptr) {
    return !!ptr;
  }
};

template <typename T>
class QueueFixture : public ::testing::Test {};

template <typename T>
class TypedQueueFixture : public ::testing::Test {
 public:
  using ValueType = typename T::ValueType;

 protected:
  // Wrap and Unwrap methods are required mostly for std::unique_ptr. With them,
  // one can make sure that unique_ptr that went into a queue is the same one as
  // the one that was pulled out.
  ValueType Wrap(const int tag) { return ValueHelper<ValueType>::Wrap(tag); }
  int Unwrap(const ValueType& object) {
    return ValueHelper<ValueType>::Unwrap(object);
  }
  // Some types may implement tracking to detect memory leaks. ValueTypehis
  // method queries whether everything is OK or not.
  static bool CheckMemoryOk() {
    return ValueHelper<ValueType>::CheckMemoryOk();
  }
  static bool HasMemoryLeakCheck() {
    return ValueHelper<ValueType>::HasMemoryLeakCheck();
  }
  static bool CheckWasNotMovedOut(const ValueType& object) {
    return ValueHelper<ValueType>::CheckWasNotMovedOut(object);
  }
};

TYPED_UTEST_SUITE_P(QueueFixture);

TYPED_UTEST_SUITE_P(TypedQueueFixture);

TYPED_UTEST_P(TypedQueueFixture, Ctr) {
  auto queue = TypeParam::Create();
  EXPECT_EQ(0U, queue->GetSizeApproximate());
}

TYPED_UTEST_P(TypedQueueFixture, Consume) {
  auto queue = TypeParam::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  EXPECT_TRUE(producer.Push(this->Wrap(1)));
  EXPECT_EQ(1, queue->GetSizeApproximate());

  typename TypeParam::ValueType value;
  EXPECT_TRUE(consumer.Pop(value));
  EXPECT_EQ(1, this->Unwrap(value));
  EXPECT_EQ(0, queue->GetSizeApproximate());
}

TYPED_UTEST_P(TypedQueueFixture, ConsumeMany) {
  auto queue = TypeParam::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  auto constexpr N = 100;

  for (int i = 0; i < N; i++) {
    auto value = this->Wrap(i);
    EXPECT_TRUE(producer.Push(std::move(value)));
    EXPECT_EQ(i + 1, queue->GetSizeApproximate());
  }

  for (int i = 0; i < N; i++) {
    typename TypeParam::ValueType value;
    EXPECT_TRUE(consumer.Pop(value));
    EXPECT_EQ(i, this->Unwrap(value));
    EXPECT_EQ(N - i - 1, queue->GetSizeApproximate());
  }
}

TYPED_UTEST_P(TypedQueueFixture, ProducerIsDead) {
  auto queue = TypeParam::Create();
  auto consumer = queue->GetConsumer();

  typename TypeParam::ValueType value;
  (void)queue->GetProducer();
  EXPECT_FALSE(consumer.Pop(value));
}

TYPED_UTEST_P(TypedQueueFixture, QueueDestroyed) {
  // This test tests that producer and consumer keep queue alive
  // even if initial shared_ptr is released
  // The real-world scenario is actually simple:
  // struct S {
  //   MpscQueue::Producer producer_;
  //   shared_ptr<MpscQueue> queue_;
  // };
  //
  // Default destructor will destroy queue_ before producer_, and if producer
  // doesn't keep queue alive, assert will be thrown.
  //
  {
    auto queue = TypeParam::Create();
    auto producer = queue->GetProducer();
    // Release queue. If destructor is actually called, it will throw assert
    queue = nullptr;
  }
  {
    auto queue = TypeParam::Create();
    auto consumer = queue->GetConsumer();
    // Release queue. If destructor is actually called, it will throw assert
    queue = nullptr;
  }
}

TYPED_UTEST_P(TypedQueueFixture, QueueCleanUp) {
  EXPECT_TRUE(this->CheckMemoryOk());
  // This test tests that if MpscQueue object is destroyed while
  // some data is inside, then all data is correctly destroyed as well.
  // This is targeted mostly at std::unique_ptr specialization, to make sure
  // that remaining pointers inside queue are correctly deleted.
  auto queue = TypeParam::Create();
  {
    auto producer = queue->GetProducer();
    EXPECT_TRUE(producer.Push(this->Wrap(1)));
    EXPECT_TRUE(producer.Push(this->Wrap(2)));
    EXPECT_TRUE(producer.Push(this->Wrap(3)));
  }
  // Objects in queue must still be alive
  if (this->HasMemoryLeakCheck()) {
    EXPECT_FALSE(this->CheckMemoryOk());
  }

  // Producer is deat at this point. 'queue' variable is the only one
  // holding MpscQueue alive. Destroying it and checking that there is no
  // memory leak
  queue = nullptr;

  // Every object in queue must have been destroyed
  EXPECT_TRUE(this->CheckMemoryOk());
}

TYPED_UTEST_P(TypedQueueFixture, Block) {
  auto queue = TypeParam::Create();

  auto consumer_task =
      utils::Async("consumer", [consumer = queue->GetConsumer(), this]() {
        typename TypeParam::ValueType value{};
        EXPECT_TRUE(consumer.Pop(value));
        EXPECT_EQ(0, this->Unwrap(value));

        EXPECT_TRUE(consumer.Pop(value));
        EXPECT_EQ(1, this->Unwrap(value));

        EXPECT_FALSE(consumer.Pop(value));
      });

  {
    auto producer = queue->GetProducer();
    EXPECT_TRUE(producer.Push(this->Wrap(0)));
    EXPECT_TRUE(producer.Push(this->Wrap(1)));
  }

  consumer_task.Get();
}

TYPED_UTEST_P(TypedQueueFixture, Noblock) {
  engine::SingleConsumerEvent wait_consumer_event_;
  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(2);

  auto consumer_task = utils::Async(
      "consumer",
      [consumer = queue->GetConsumer(), &wait_consumer_event_, this]() {
        typename TypeParam::ValueType value{};

        EXPECT_FALSE(consumer.PopNoblock(value));
        wait_consumer_event_.Send();

        while (!consumer.PopNoblock(value)) {
          engine::Yield();
        }
        EXPECT_EQ(0, this->Unwrap(value));

        EXPECT_TRUE(consumer.PopNoblock(value));
        EXPECT_EQ(1, this->Unwrap(value));
      });

  // For consumer_task try to PopNoblock
  [[maybe_unused]] auto result = wait_consumer_event_.WaitForEvent();

  {
    auto producer = queue->GetProducer();
    EXPECT_TRUE(producer.PushNoblock(this->Wrap(0)));
    EXPECT_TRUE(producer.PushNoblock(this->Wrap(1)));

    auto value = this->Wrap(2);
    EXPECT_FALSE(producer.PushNoblock(std::move(value)));
    EXPECT_TRUE(this->CheckWasNotMovedOut(value));
  }

  consumer_task.Get();
}

TYPED_UTEST_P(TypedQueueFixture, NotMovedValueOnFalse) {
  engine::SingleConsumerEvent wait_consumer_event_;
  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(2);

  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();
  EXPECT_TRUE(producer.PushNoblock(this->Wrap(0)));
  EXPECT_TRUE(producer.PushNoblock(this->Wrap(1)));

  auto value = this->Wrap(2);
  EXPECT_FALSE(producer.PushNoblock(std::move(value)));
  EXPECT_TRUE(this->CheckWasNotMovedOut(value));

  engine::current_task::GetCancellationToken().RequestCancel();
  EXPECT_FALSE(producer.Push(std::move(value)));
  EXPECT_TRUE(this->CheckWasNotMovedOut(value));
}

REGISTER_TYPED_UTEST_SUITE_P(TypedQueueFixture, Ctr, Consume, ConsumeMany,
                             ProducerIsDead, QueueDestroyed, QueueCleanUp,
                             Block, Noblock, NotMovedValueOnFalse);

TYPED_UTEST_P(QueueFixture, BlockMulti) {
  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(0);
  auto producer = queue->GetProducer();
  auto consumer = queue->GetConsumer();

  auto task1 = utils::Async("pusher", [&]() { ASSERT_TRUE(producer.Push(1)); });
  auto task2 = utils::Async("pusher", [&]() { ASSERT_TRUE(producer.Push(1)); });

  // task is blocked

  int value{0};
  ASSERT_FALSE(consumer.PopNoblock(value));

  queue->SetSoftMaxSize(2);

  ASSERT_TRUE(consumer.Pop(value));
  EXPECT_EQ(value, 1);

  ASSERT_TRUE(consumer.Pop(value));
  EXPECT_EQ(value, 1);

  ASSERT_FALSE(consumer.PopNoblock(value));
}

TYPED_UTEST_P(QueueFixture, BlockConsumerWithProducer) {
  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(2);
  auto consumer = queue->GetConsumer();
  (void)queue->GetProducer();
  auto producer = queue->GetProducer();

  int value{0};
  auto consumer_task =
      utils::Async("consumer", [&] { ASSERT_TRUE(consumer.Pop(value)); });

  ASSERT_TRUE(producer.Push(1));
  consumer_task.Get();
  ASSERT_EQ(value, 1);
}

TYPED_UTEST_P(QueueFixture, ProducersCreation) {
  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(1);

  auto consumer = queue->GetConsumer();
  int value{0};

  {
    auto producer_1 = queue->GetProducer();
    ASSERT_TRUE(producer_1.PushNoblock(1));

    auto producer_2 = queue->GetProducer();
    auto task = utils::Async("pusher", [&] { return producer_2.Push(2); });

    ASSERT_TRUE(consumer.PopNoblock(value));
    EXPECT_EQ(value, 1);
    ASSERT_TRUE(consumer.Pop(value));
    EXPECT_EQ(value, 2);

    ASSERT_TRUE(task.Get());
  }

  auto producer_3 = queue->GetProducer();
  ASSERT_TRUE(producer_3.PushNoblock(3));
  ASSERT_FALSE(producer_3.PushNoblock(4));

  ASSERT_TRUE(consumer.PopNoblock(value));
  EXPECT_EQ(value, 3);

  EXPECT_EQ(queue->GetSizeApproximate(), 0);
}

TYPED_UTEST_P_MT(QueueFixture, ManyProducers, 4) {
  constexpr std::size_t kProducersCount = 3;
  constexpr std::size_t kMessageCount = 1000;

  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(kMessageCount);
  auto consumer = queue->GetConsumer();

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kProducersCount);

  std::vector<typename TypeParam::Producer> producers;
  producers.reserve(kProducersCount);
  for (std::size_t i = 0; i < kProducersCount; ++i) {
    producers.push_back(queue->GetProducer());
  }

  for (std::size_t i = 0; i < kProducersCount; ++i) {
    tasks.push_back(utils::Async("pusher", [&producer = producers[i], i] {
      for (std::size_t message = kMessageCount * i;
           message < (i + 1) * kMessageCount; ++message) {
        ASSERT_TRUE(producer.Push(static_cast<int>(message)));
      }
    }));
  }

  int messages = kProducersCount * kMessageCount;
  std::vector<int> consumed_messages(messages);
  int value{0};
  while (messages-- > 0) {
    ASSERT_TRUE(consumer.Pop(value));
    ++consumed_messages[value];
  }

  for (auto& task : tasks) {
    task.Get();
  }

  ASSERT_TRUE(std::all_of(consumed_messages.begin(), consumed_messages.end(),
                          [](auto item) { return (item == 1); }));
  EXPECT_EQ(queue->GetSizeApproximate(), 0);
}

TYPED_UTEST_P_MT(QueueFixture, MultiProducerToken, 4) {
  constexpr std::size_t kProducersCount = 3;
  constexpr std::size_t kMessageCount = 1000;

  auto queue = TypeParam::Create();
  queue->SetSoftMaxSize(kMessageCount);
  auto consumer = queue->GetConsumer();

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kProducersCount);

  auto producer = queue->GetMultiProducer();

  for (std::size_t i = 0; i < kProducersCount; ++i) {
    tasks.push_back(utils::Async("pusher", [&producer, i] {
      for (std::size_t message = kMessageCount * i;
           message < (i + 1) * kMessageCount; ++message) {
        ASSERT_TRUE(producer.Push(static_cast<int>(message)));
      }
    }));
  }

  int messages = kProducersCount * kMessageCount;
  std::vector<int> consumed_messages(messages);
  int value{0};
  while (messages-- > 0) {
    ASSERT_TRUE(consumer.Pop(value));
    ++consumed_messages[value];
  }

  for (auto& task : tasks) {
    task.Get();
  }

  ASSERT_TRUE(std::all_of(consumed_messages.begin(), consumed_messages.end(),
                          [](auto item) { return (item == 1); }));
  EXPECT_EQ(queue->GetSizeApproximate(), 0);
}

REGISTER_TYPED_UTEST_SUITE_P(QueueFixture, BlockMulti,
                             BlockConsumerWithProducer, ManyProducers,
                             MultiProducerToken, ProducersCreation);

USERVER_NAMESPACE_END
