#include <gtest/gtest-typed-test.h>
#include <boost/core/ignore_unused.hpp>
#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/spsc_queue.hpp>
#include <utest/utest.hpp>

namespace {
// This class tracks the number of created and destroyed objects
struct RefCountData final {
  int val{0};

  // signed to allow for errorneous deletion to be detected.
  static std::atomic<int> objects_count;

  RefCountData(const int value = 0) : val(value) { objects_count.fetch_add(1); }
  ~RefCountData() { objects_count.fetch_sub(1); }
  // Non-copyable, non-movable
  RefCountData(const RefCountData&) = delete;
  void operator=(const RefCountData&) = delete;
  RefCountData(RefCountData&&) = delete;
  void operator=(RefCountData&&) = delete;
};

std::atomic<int> RefCountData::objects_count{0};
}  // namespace

/// SpscValueHelper and it's overloads provide methods:
/// 'Wrap', that accepts and int tag and must return an object of type T.
/// 'Unwrap', that extracts tag back. Unwrap(Wrap(X)) == X
template <typename T>
struct SpscValueHelper {};

/// Overload for int
template <>
struct SpscValueHelper<int> {
  static int Wrap(int tag) { return tag; }
  static int Unwrap(int tag) { return tag; }
  static bool HasMemoryLeakCheck() { return false; }
  static bool CheckMemoryOk() { return true; }
};

template <>
struct SpscValueHelper<std::unique_ptr<int>> {
  static auto Wrap(int tag) { return std::make_unique<int>(tag); }
  static auto Unwrap(const std::unique_ptr<int>& ptr) { return *ptr; }
  static bool HasMemoryLeakCheck() { return false; }
  static bool CheckMemoryOk() { return true; }
};

template <>
struct SpscValueHelper<std::unique_ptr<RefCountData>> {
  static auto Wrap(int tag) { return std::make_unique<RefCountData>(tag); }
  static auto Unwrap(const std::unique_ptr<RefCountData>& data) {
    return data->val;
  }
  // Checks that no object was leaked
  static bool CheckMemoryOk() {
    return RefCountData::objects_count.load() == 0;
  }
  static bool HasMemoryLeakCheck() { return true; }
};

template <typename T>
class SpscQueueFixture : public ::testing::Test {
 protected:
  // Wrap and Unwrap methods are required mostly for std::unique_ptr. With them,
  // one can make sure that unique_ptr that went into a queue is the same one as
  // the one that was pulled out.
  T Wrap(const int tag) { return SpscValueHelper<T>::Wrap(tag); }
  int Unwrap(const T& object) { return SpscValueHelper<T>::Unwrap(object); }
  // Some types may implement tracking to detect memory leaks. This method
  // queries whether everything is OK or not.
  static bool CheckMemoryOk() { return SpscValueHelper<T>::CheckMemoryOk(); }
  static bool HasMemoryLeakCheck() {
    return SpscValueHelper<T>::HasMemoryLeakCheck();
  }
};

using SpscTestTypes =
    testing::Types<int, std::unique_ptr<int>, std::unique_ptr<RefCountData>>;

TYPED_TEST_CASE(SpscQueueFixture, SpscTestTypes);

TYPED_TEST(SpscQueueFixture, Ctr) {
  auto queue = engine::SpscQueue<TypeParam>::Create();
  EXPECT_EQ(0u, queue->Size());
}

TYPED_TEST(SpscQueueFixture, Consume) {
  auto queue = engine::SpscQueue<TypeParam>::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  EXPECT_TRUE(producer.Push(this->Wrap(1)));
  EXPECT_EQ(1, queue->Size());

  TypeParam value;
  EXPECT_TRUE(consumer.Pop(value));
  EXPECT_EQ(1, this->Unwrap(value));
  EXPECT_EQ(0, queue->Size());
}

TYPED_TEST(SpscQueueFixture, ConsumeMany) {
  auto queue = engine::SpscQueue<TypeParam>::Create();
  auto consumer = queue->GetConsumer();
  auto producer = queue->GetProducer();

  auto constexpr N = 100;

  for (int i = 0; i < N; i++) {
    auto value = this->Wrap(i);
    EXPECT_TRUE(producer.Push(std::move(value)));
    EXPECT_EQ(i + 1, queue->Size());
  }

  for (int i = 0; i < N; i++) {
    TypeParam value;
    EXPECT_TRUE(consumer.Pop(value));
    EXPECT_EQ(i, this->Unwrap(value));
    EXPECT_EQ(N - i - 1, queue->Size());
  }
}

TYPED_TEST(SpscQueueFixture, ProducerIsDead) {
  auto queue = engine::SpscQueue<TypeParam>::Create();
  auto consumer = queue->GetConsumer();

  TypeParam value;
  boost::ignore_unused(queue->GetProducer());
  EXPECT_FALSE(consumer.Pop(value));
}

TYPED_TEST(SpscQueueFixture, ConsumerIsDead) {
  auto queue = engine::SpscQueue<TypeParam>::Create();
  auto producer = queue->GetProducer();

  boost::ignore_unused(queue->GetConsumer());
  EXPECT_FALSE(producer.Push(this->Wrap(0)));
}

TYPED_TEST(SpscQueueFixture, QueueDestroyed) {
  // This test tests that producer and consumer keep queue alive
  // even if initial shared_ptr is released
  // The real-world scenario is actually simple:
  // struct S {
  //   SpscQueue::Producer producer_;
  //   shared_ptr<SpscQueue> queue_;
  // };
  //
  // Default destructor will destroy queue_ before producer_, and if producer
  // doesn't keep queue alive, assert will be thrown.
  //
  {
    auto queue = engine::SpscQueue<TypeParam>::Create();
    auto producer = queue->GetProducer();
    // Release queue. If destructor is actually called, it will throw assert
    queue = nullptr;
  }
  {
    auto queue = engine::SpscQueue<TypeParam>::Create();
    auto consumer = queue->GetConsumer();
    // Release queue. If destructor is actually called, it will throw assert
    queue = nullptr;
  }
}

TYPED_TEST(SpscQueueFixture, QueueCleanUp) {
  EXPECT_TRUE(this->CheckMemoryOk());
  // This test tests that if SpscQueue object is destroyed while
  // some data is inside, then all data is correctly destroyed as well.
  // This is targeted mostly at std::unique_ptr specialization, to make sure
  // that remaining pointers inside queue are correctly deleted.
  auto queue = engine::SpscQueue<TypeParam>::Create();
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
  // holding SpscQueue alive. Destroying it and checking that there is no
  // memory leak
  queue = nullptr;

  // Every object in queue must have been destroyed
  EXPECT_TRUE(this->CheckMemoryOk());
}

TYPED_TEST(SpscQueueFixture, Block) {
  RunInCoro([this] {
    auto queue = engine::SpscQueue<TypeParam>::Create();

    auto consumer_task =
        engine::impl::Async([consumer = queue->GetConsumer(), this]() mutable {
          TypeParam value{};
          EXPECT_TRUE(consumer.Pop(value));
          EXPECT_EQ(0, this->Unwrap(value));

          EXPECT_TRUE(consumer.Pop(value));
          EXPECT_EQ(1, this->Unwrap(value));

          EXPECT_FALSE(consumer.Pop(value));
        });

    engine::Yield();
    engine::Yield();

    {
      auto producer = queue->GetProducer();
      EXPECT_TRUE(producer.Push(this->Wrap(0)));
      engine::Yield();
      EXPECT_TRUE(producer.Push(this->Wrap(1)));
    }

    consumer_task.Get();
  });
}

TYPED_TEST(SpscQueueFixture, Noblock) {
  RunInCoro([this] {
    auto queue = engine::SpscQueue<TypeParam>::Create();
    queue->SetMaxLength(2);

    auto consumer_task =
        engine::impl::Async([consumer = queue->GetConsumer(), this]() mutable {
          TypeParam value{};
          size_t i = 0;
          while (!consumer.PopNoblock(value)) {
            ++i;
            engine::Yield();
          }
          EXPECT_EQ(0, this->Unwrap(value));
          EXPECT_NE(0, i);

          EXPECT_TRUE(consumer.PopNoblock(value));
          EXPECT_EQ(1, this->Unwrap(value));
        });

    engine::Yield();
    engine::Yield();

    {
      auto producer = queue->GetProducer();
      EXPECT_TRUE(producer.PushNoblock(this->Wrap(0)));
      EXPECT_TRUE(producer.PushNoblock(this->Wrap(1)));
      EXPECT_FALSE(producer.PushNoblock(this->Wrap(2)));
    }

    consumer_task.Get();
  });
}
