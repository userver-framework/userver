#include <storages/redis/subscription_token.hpp>

#include <stdexcept>

#include <engine/task/task_with_result.hpp>
#include <tracing/span.hpp>
#include <utils/async.hpp>

#include "subscription_queue.hpp"

namespace storages::redis {

SubscriptionToken::SubscriptionToken() = default;

SubscriptionToken::~SubscriptionToken() = default;

SubscriptionToken& SubscriptionToken::operator=(SubscriptionToken&&) noexcept =
    default;

SubscriptionToken::SubscriptionToken(SubscriptionToken&& other) noexcept =
    default;

SubscriptionToken::SubscriptionToken(
    std::unique_ptr<SubscriptionTokenImplBase>&& implementation)
    : impl_(std::move(implementation)) {}

void SubscriptionToken::SetMaxQueueLength(size_t length) {
  if (!impl_) throw std::runtime_error("SubscriptionToken is not initialized");
  impl_->SetMaxQueueLength(length);
}

void SubscriptionToken::Unsubscribe() {
  if (!impl_) return;
  impl_->Unsubscribe();
}

}  // namespace storages::redis
