#include <userver/concurrent/mpsc_queue.hpp>

#include <gmock/gmock.h>

#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

#include "mp_queue_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
using TestTypes = testing::Types<
    concurrent::MpscQueue<int>,
    concurrent::MpscQueue<std::unique_ptr<int>>,
    concurrent::MpscQueue<std::unique_ptr<RefCountData>>>;

constexpr std::size_t kProducersCount = 4;
constexpr std::size_t kMessageCount = 1000;
}  // namespace

INSTANTIATE_TYPED_UTEST_SUITE_P(MpscQueue, QueueFixture, concurrent::MpscQueue<int>);

INSTANTIATE_TYPED_UTEST_SUITE_P(MpscQueue, TypedQueueFixture, TestTypes);

UTEST(MpscQueue, ConsumerIsDead) {
    auto queue = concurrent::MpscQueue<int>::Create();
    auto producer = queue->GetProducer();

    (void)(queue->GetConsumer());
    EXPECT_FALSE(producer.Push(0));
}

UTEST(MpscQueue, NoCrashOnProducerReuse) {
    auto queue = concurrent::MpscQueue<int>::Create();
    auto producer = queue->GetProducer();

    queue = concurrent::MpscQueue<int>::Create();
    producer = queue->GetProducer();
}

UTEST(MpscQueue, NoCrashOnConsumerReuse) {
    auto queue = concurrent::MpscQueue<int>::Create();
    auto consumer = queue->GetConsumer();

    queue = concurrent::MpscQueue<int>::Create();
    consumer = queue->GetConsumer();
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

namespace {

/// [close sample]
using FooItem = std::string;

class FooProcessor final {
public:
    explicit FooProcessor(std::size_t max_size)
        : queue_(Queue::Create(max_size)), queue_producer_(queue_->GetMultiProducer()) {
        // There is no use for a Span in a task that lives until the service stops.
        // The task should be Critical, because the whole service (not just a single request) depends on it.
        consumer_task_ =
            engine::CriticalAsyncNoSpan([&, consumer = queue_->GetConsumer()] { ConsumerTaskLoop(consumer); });
    }

    ~FooProcessor() {
        // When the last producer is dropped, and the remaining items are consumed,
        // the queue becomes "closed for pop", and `Pop` starts returning `false`.
        std::move(queue_producer_).Reset();
        // Allow the task to process the remaining elements.
        // If the current task is cancelled, we will honor the cancellation and stop waiting.
        // (In this case, the task will be automatically cancelled and awaited in the destructor.)
        // You can also provide a deadline until which we will try to process the items.
        [[maybe_unused]] const bool success = consumer_task_.WaitNothrow();
    }

    void ProcessAsync(FooItem&& item) {
        // If the queue has a max size limit, pushing will block upon reaching it.
        // This provides backpressure, which, when utilized effectively, can improve reliability
        // of processing pipelines.
        const bool success = queue_producer_.Push(std::move(item));
        if (!success) {
            // If the task is waiting for free capacity and is cancelled, Push will return `false`.
            // It will also return `false` if all the producers are dropped, which should not be possible in our case.
            UINVARIANT(engine::current_task::ShouldCancel(), "Unexpected Push failure, is consumer dropped?");
            throw engine::WaitInterruptedException(engine::current_task::CancellationReason());
        }
    }

private:
    using Queue = concurrent::MpscQueue<FooItem>;

    void ConsumerTaskLoop(const Queue::Consumer& consumer) {
        FooItem item;
        // When the last producer is dropped, and the remaining items are consumed,
        // the queue becomes "closed for pop", and `Pop` starts returning `false`.
        // If the current task is cancelled, `consumer.Pop` will return `false` as well.
        while (consumer.Pop(item)) {
            // [optional] On task cancellation, don't keep processing the remaining elements,
            // just destroy them.
            if (engine::current_task::ShouldCancel()) break;

            try {
                // You might want to measure the time it takes to process an item.
                const tracing::Span process_span{"foo-processor"};

                DoProcess(item);
            } catch (const std::exception& ex) {
                // Failure to handle a single item should typically not stop the whole queue
                // operation. Although backoff and fallback strategies may vary.
                LOG_ERROR() << "Processing of item '" << item << "' failed: " << ex;
            }
        }
    }

    void DoProcess(const FooItem& item);

    std::shared_ptr<Queue> queue_;
    Queue::MultiProducer queue_producer_;
    // `consumer_task_` MUST be the last field so that it is awaited before
    // the other fields (which are used in the task) are destroyed.
    engine::TaskWithResult<void> consumer_task_;
};
/// [close sample]

std::vector<std::string> foo_items;

void FooProcessor::DoProcess(const FooItem& item) { foo_items.push_back(item); }

}  // namespace

UTEST(MpscQueue, ProcessingRemainingItemsSample) {
    ASSERT_EQ(GetThreadCount(), 1) << "In this test we can observe the exact moments of task switching, because there "
                                      "is a single TaskProcessor thread. We also don't need protecting 'foo_items'";
    foo_items.clear();

    {
        FooProcessor processor{2};
        processor.ProcessAsync("a");
        processor.ProcessAsync("b");
        EXPECT_THAT(foo_items, testing::IsEmpty()) << "The consumer task has not had the chance to run yet";

        processor.ProcessAsync("c");
        EXPECT_THAT(foo_items, testing::ElementsAre("a", "b"))
            << "At Push the current (parent) task should yield to the consumer task, until it processes some items";

        engine::Yield();
        EXPECT_THAT(foo_items, testing::ElementsAre("a", "b", "c"));

        processor.ProcessAsync("d");
        processor.ProcessAsync("e");
        EXPECT_THAT(foo_items, testing::ElementsAre("a", "b", "c"))
            << "Again, the consumer task has not had the chance to run and process the new items yet";
    }

    EXPECT_THAT(foo_items, testing::ElementsAre("a", "b", "c", "d", "e"))
        << "FooProcessor's destructor should await processing of the remaining items";
}

UTEST(MpscQueue, ProcessingRemainingItemsCancelled) {
    ASSERT_EQ(GetThreadCount(), 1) << "In this test we can observe the exact moments of task switching, because there "
                                      "is a single TaskProcessor thread. We also don't need protecting 'foo_items'";
    foo_items.clear();

    {
        FooProcessor processor{concurrent::MpscQueue<FooItem>::kUnbounded};
        processor.ProcessAsync("a");
        processor.ProcessAsync("b");

        engine::current_task::RequestCancel();
    }

    EXPECT_THAT(foo_items, testing::IsEmpty());
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
        producers_tasks.push_back(utils::Async("producer", [&producer = producers[i], i] {
            for (std::size_t message = i * kMessageCount; message < (i + 1) * kMessageCount; ++message) {
                ASSERT_TRUE(producer.Push(std::size_t{message}));
            }
        }));
    }

    std::vector<std::size_t> consumed_messages(kMessageCount * kProducersCount, 0);
    auto consumer_task = utils::Async("consumer", [&] {
        std::vector<std::size_t> previous(kProducersCount, 0);

        std::size_t value{};
        while (consumer.Pop(value)) {
            const std::size_t step = value / kMessageCount;
            ASSERT_TRUE(previous[step] == 0 || previous[step] < value % kMessageCount);
            previous[step] = value % kMessageCount;
            ++consumed_messages[value];
        }
    });

    for (auto& task : producers_tasks) {
        task.Get();
    }
    producers.clear();

    consumer_task.Get();

    ASSERT_TRUE(std::all_of(consumed_messages.begin(), consumed_messages.end(), [](int item) { return (item == 1); }));
}

UTEST_MT(MpscQueue, ProducerRace, kProducersCount + 1) {
    const auto test_deadline = engine::Deadline::FromDuration(std::chrono::milliseconds(100));
    auto queue = concurrent::MpscQueue<std::size_t>::Create();

    auto consumer = queue->GetConsumer();
    auto producers = utils::GenerateFixedArray(kProducersCount, [&](std::size_t) { return queue->GetProducer(); });

    while (!test_deadline.IsReached()) {
        auto consumer_task = engine::AsyncNoSpan([&consumer] {
            for (std::size_t i = 0; i < kProducersCount; ++i) {
                std::size_t item{};
                // If there queue is buggy (loses wakeups), then we'll eventually hang here until the deadline.
                ASSERT_TRUE(consumer.Pop(item, engine::Deadline::FromDuration(utest::kMaxTestWaitTime)));
            }
        });

        std::atomic<bool> go{false};

        auto producer_tasks = utils::GenerateFixedArray(kProducersCount, [&](std::size_t i) {
            return engine::AsyncNoSpan([&producers, &go, i] {
                while (!go.load()) {
                    // Busy loop
                }
                ASSERT_TRUE(producers[i].Push(std::size_t{i}));
            });
        });

        go = true;

        UASSERT_NO_THROW(consumer_task.Get());
        for (auto& producer_task : producer_tasks) {
            UASSERT_NO_THROW(producer_task.Get());
        }
    }
}

USERVER_NAMESPACE_END
