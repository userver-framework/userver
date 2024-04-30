#pragma once

#include <cstddef>

#include <moodycamel/blockingconcurrentqueue.h>
#include <moodycamel/concurrentqueue.h>
#include <moodycamel/lightweightsemaphore.h>

#include <userver/utils/span.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {
class TaskContext;
}  // namespace impl


class NewGlobalQueue final {
  public:
    NewGlobalQueue(std::size_t consumers_count);
    std::size_t GetSizeApproximate() const noexcept { return size_; }
    struct Token {
      private:
      friend NewGlobalQueue;
      Token(moodycamel::ConsumerToken&& moodycamel_token, const std::size_t index): index_(index), moodycamel_token_(std::move(moodycamel_token))  {}
      const std::size_t index_;
      moodycamel::ConsumerToken moodycamel_token_;
    };


    void Push(impl::TaskContext* ctx); 
    void PushBulk(const utils::span<impl::TaskContext*> buffer);
    void Push(Token& token, impl::TaskContext* ctx); 
    void PushBulk(Token& token, const utils::span<impl::TaskContext*> buffer);
    impl::TaskContext* TryPop(Token& token);
    std::size_t PopBulk(Token& token, utils::span<impl::TaskContext*> buffer);
    Token CreateConsumerToken();

  private:
   void DoPush(const std::size_t index, const utils::span<impl::TaskContext*> buffer);
   // void DoPushBulk(const std::size_t index, impl::TaskContext* ctx);
   std::int64_t GetCountersSum();
   std::size_t GetRandomIndex();
  const std::size_t consumers_count_;
  moodycamel::ConcurrentQueue<impl::TaskContext*> queue_;
  utils::FixedArray<std::atomic<std::int64_t>> shared_counters_;
  std::atomic<std::size_t> token_order_{0};
  std::atomic<std::size_t> size_{0};
};

}  // namespace engine

USERVER_NAMESPACE_END
