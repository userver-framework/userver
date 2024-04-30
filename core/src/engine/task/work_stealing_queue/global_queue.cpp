#include <engine/task/work_stealing_queue/global_queue.hpp>
#include <moodycamel/concurrentqueue.h>
#include <cstddef>
#include "userver/utils/rand.hpp"
#include "userver/utils/span.hpp"
USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
    std::size_t CalculateDistibutedCountersCount(const std::size_t consumers_count) {
        return std::min({consumers_count * 2, consumers_count + 2, static_cast<std::size_t>(std::thread::hardware_concurrency())});
    } 
}

NewGlobalQueue::NewGlobalQueue(std::size_t consumers_count): consumers_count_(consumers_count), shared_counters_(CalculateDistibutedCountersCount(consumers_count), 0) {
}

void NewGlobalQueue::Push(impl::TaskContext* ctx) {
    DoPush(GetRandomIndex(), utils::span(&ctx, 1));
}
void NewGlobalQueue::PushBulk(const utils::span<impl::TaskContext*> buffer) {
    DoPush(GetRandomIndex(), buffer);
}

void NewGlobalQueue::Push(Token& token, impl::TaskContext* ctx) {
    DoPush(token.index_, utils::span(&ctx, 1));
}
void NewGlobalQueue::PushBulk(Token& token, const utils::span<impl::TaskContext*> buffer) {
    DoPush(token.index_, buffer);
}
impl::TaskContext* NewGlobalQueue::TryPop(Token& token) {
    impl::TaskContext* context = nullptr;
    PopBulk(token, utils::span(&context, 1));
    return context;
}
std::size_t NewGlobalQueue::PopBulk(Token& token, utils::span<impl::TaskContext*> buffer) {
    const auto shared_index = token.index_;
    std::size_t i = 0;
    while (true) {
      if (i++ >= 3) {
        i = 0;
        if (GetCountersSum() <= 0) {
            break;
        }
      }
      std::size_t bulk_size =
          queue_.try_dequeue_bulk(token.moodycamel_token_, buffer.data(), buffer.size());
      if (bulk_size > 0) {
        shared_counters_[shared_index].fetch_sub(bulk_size, std::memory_order_release);
        return bulk_size;
      }
    }
    return 0;
}

NewGlobalQueue::Token NewGlobalQueue::CreateConsumerToken() {
    auto index = token_order_.load();
    while (!token_order_.compare_exchange_strong(index, index + 1)) {}
    return NewGlobalQueue::Token{moodycamel::ConsumerToken(queue_), index % shared_counters_.size()};
}

void NewGlobalQueue::DoPush(const std::size_t index, const utils::span<impl::TaskContext*> buffer) {
    shared_counters_[index].fetch_add(buffer.size(), std::memory_order_release);
    queue_.enqueue_bulk(buffer.data(), buffer.size());
}


std::int64_t NewGlobalQueue::GetCountersSum() {
    std::int64_t sum{0};
    for (const auto& counter : shared_counters_) {
        sum += counter.load(std::memory_order_acquire);
    }
    thread_local std::size_t steps_count = 0;
    steps_count++;
    if (steps_count >= consumers_count_) {
        size_.store(static_cast<std::size_t>(std::max<std::int64_t>(sum, 0)));
        steps_count = 0;
    }
    return sum;
}


std::size_t NewGlobalQueue::GetRandomIndex() {
    thread_local std::size_t index = utils::Rand();
    return (index++) % shared_counters_.size();
}

}  // namespace engine

USERVER_NAMESPACE_END
