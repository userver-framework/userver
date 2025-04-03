#pragma once

#include <cstddef>
#include <optional>

#include <moodycamel/concurrentqueue.h>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl

class GlobalQueue final {
public:
    struct Token {
    private:
        friend GlobalQueue;
        Token(moodycamel::ConsumerToken&& moodycamel_token, const std::size_t index)
            : index_(index), moodycamel_token_(std::move(moodycamel_token)) {}

        const std::size_t index_;
        moodycamel::ConsumerToken moodycamel_token_;
    };

    explicit GlobalQueue(std::size_t consumers_count);

    std::size_t GetSizeApproximateDelayed() const noexcept { return size_; }

    std::size_t GetSizeApproximate() const noexcept { return queue_.size_approx(); }

    void Push(impl::TaskContext* ctx);

    void PushBulk(const utils::span<impl::TaskContext*> buffer);

    void Push(Token& token, impl::TaskContext* ctx);

    void PushBulk(Token& token, const utils::span<impl::TaskContext*> buffer);

    impl::TaskContext* TryPop(Token& token);

    std::size_t PopBulk(Token& token, utils::span<impl::TaskContext*> buffer);

    Token CreateConsumerToken();

private:
    void DoPush(const std::size_t index, const utils::span<impl::TaskContext*> buffer);

    std::int64_t GetCountersSum() const noexcept;

    std::int64_t GetCountersSumAndUpdateSize();

    void UpdateSize(std::optional<std::size_t> size);

    std::size_t GetRandomIndex();

    const std::size_t consumers_count_;
    moodycamel::ConcurrentQueue<impl::TaskContext*> queue_;
    utils::FixedArray<concurrent::impl::InterferenceShield<std::atomic<std::int64_t>>> shared_counters_;
    std::atomic<std::size_t> token_order_{0};
    std::atomic<std::size_t> size_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
