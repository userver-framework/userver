#include <engine/task/work_stealing_queue/global_queue.hpp>

#include <atomic>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {
std::size_t CalculateDistributedCountersCount(const std::size_t consumers_count) {
    return std::min(
        {consumers_count * 2, consumers_count + 2, static_cast<std::size_t>(std::thread::hardware_concurrency())}
    );
}
}  // namespace

GlobalQueue::GlobalQueue(std::size_t consumers_count)
    : consumers_count_(consumers_count), shared_counters_(CalculateDistributedCountersCount(consumers_count), 0) {}

void GlobalQueue::Push(impl::TaskContext* ctx) { DoPush(GetRandomIndex(), utils::span(&ctx, 1)); }

void GlobalQueue::PushBulk(const utils::span<impl::TaskContext*> buffer) { DoPush(GetRandomIndex(), buffer); }

void GlobalQueue::Push(Token& token, impl::TaskContext* ctx) { DoPush(token.index_, utils::span(&ctx, 1)); }

void GlobalQueue::PushBulk(Token& token, const utils::span<impl::TaskContext*> buffer) { DoPush(token.index_, buffer); }

impl::TaskContext* GlobalQueue::TryPop(Token& token) {
    impl::TaskContext* context = nullptr;
    PopBulk(token, utils::span(&context, 1));
    return context;
}

std::size_t GlobalQueue::PopBulk(Token& token, utils::span<impl::TaskContext*> buffer) {
    const auto shared_index = token.index_;
    std::size_t i = 0;
    while (true) {
        if (i++ >= 3) {
            i = 0;
            if (GetCountersSumAndUpdateSize() <= 0) {
                break;
            }
        }
        std::size_t bulk_size = queue_.try_dequeue_bulk(token.moodycamel_token_, buffer.data(), buffer.size());
        if (bulk_size > 0) {
            shared_counters_[shared_index]->fetch_sub(bulk_size, std::memory_order_release);
            UpdateSize(std::nullopt);
            return bulk_size;
        }
    }
    return 0;
}

GlobalQueue::Token GlobalQueue::CreateConsumerToken() {
    auto index = token_order_.fetch_add(1);
    return GlobalQueue::Token{moodycamel::ConsumerToken(queue_), index % shared_counters_.size()};
}

void GlobalQueue::DoPush(const std::size_t index, const utils::span<impl::TaskContext*> buffer) {
    shared_counters_[index]->fetch_add(buffer.size(), std::memory_order_release);
    queue_.enqueue_bulk(buffer.data(), buffer.size());
}

std::int64_t GlobalQueue::GetCountersSum() const noexcept {
    std::int64_t sum{0};
    for (const auto& counter : shared_counters_) {
        sum += counter->load(std::memory_order_acquire);
    }
    return sum;
}

std::int64_t GlobalQueue::GetCountersSumAndUpdateSize() {
    std::int64_t sum = GetCountersSum();
    UpdateSize(std::max<std::int64_t>(sum, 0));
    return sum;
}

void GlobalQueue::UpdateSize(std::optional<std::size_t> size) {
    thread_local std::atomic<std::size_t> steps_count = 0;
    auto curr_count = steps_count.fetch_add(1, std::memory_order_relaxed);
    curr_count++;

    if (size.has_value()) {
        if (curr_count >= consumers_count_) {
            steps_count.store(0, std::memory_order_relaxed);
            size_.store(size.value(), std::memory_order_relaxed);
        }
    } else if (curr_count >= consumers_count_ * consumers_count_) {
        steps_count.store(0, std::memory_order_relaxed);
        size_.store(static_cast<std::size_t>(std::max<std::int64_t>(GetCountersSum(), 0)), std::memory_order_relaxed);
    }
}

std::size_t GlobalQueue::GetRandomIndex() {
    thread_local std::atomic<std::size_t> random_index = utils::Rand();
    auto index = random_index.fetch_add(1, std::memory_order_relaxed);
    return index % shared_counters_.size();
}

}  // namespace engine

USERVER_NAMESPACE_END
