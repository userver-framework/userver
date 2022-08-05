#include "wait_list_light.hpp"

#include <atomic>
#include <cstddef>

#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

#include <concurrent/intrusive_walkable_pool.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

struct WaitListLight::Impl final {
  std::atomic<concurrent::impl::TaggedPtr<TaskContext>> waiter{nullptr};
};

WaitListLight::WaitListLight() noexcept : impl_() {}

WaitListLight::~WaitListLight() {
  UASSERT_MSG(impl_->waiter.load().GetDataPtr() == nullptr,
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());
  UASSERT_MSG(!impl_->waiter.load(std::memory_order_relaxed).GetDataPtr(),
              "Another coroutine is already waiting on this WaitListLight");
  LOG_TRACE() << "Appending, use_count=" << context->use_count();

  // We can only fit 16-bit epoch into the atomic. This means that we allow no
  // more than 2^16-1 coroutine sleep sessions during a single racy WakeupOne,
  // as opposed to the regular 2^32 ones. Not a big deal, we use boost::lockfree
  // queues with 16-bit tags anyway.
  const auto cut_epoch =
      static_cast<std::uint16_t>(utils::UnderlyingValue(context->GetEpoch()));

  // Keep a ref-counter to ensure that WakeupOne can complete safely in parallel
  // with the waiting task being cancelled, Remove-d and stopped.
  auto* const ctx = context.detach();

  impl_->waiter.store({ctx, cut_epoch}, std::memory_order_seq_cst);
}

void WaitListLight::WakeupOne() {
  const auto context_and_epoch = impl_->waiter.exchange(nullptr);
  auto* const ctx = context_and_epoch.GetDataPtr();
  if (ctx == nullptr) return;
  boost::intrusive_ptr<TaskContext> context{ctx, /*add_ref=*/false};

  const auto cut_epoch = context_and_epoch.GetTag();
  // We cheat a bit, stealing the higher 16 bits from the current epoch. This
  // means that we only have 16 bits of epoch protection here, again. (See also
  // comments in Append.)
  const auto epoch = static_cast<SleepState::Epoch>(
      (utils::UnderlyingValue(context->GetEpoch()) & 0xffff0000) | cut_epoch);

  LOG_TRACE() << "Waking up, use_count=" << context->use_count();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());

  const auto context_and_epoch = impl_->waiter.exchange(nullptr);
  auto* const ctx = context_and_epoch.GetDataPtr();

  if (ctx == nullptr) return;
  UASSERT_MSG(ctx == &context,
              "Attempting to wait in a single WaitListLight from multiple "
              "coroutines");
  UASSERT_MSG(
      context_and_epoch.GetTag() == static_cast<std::uint16_t>(ctx->GetEpoch()),
      "The stored sleep epoch is outdated");

  LOG_TRACE() << "Remove";
  boost::intrusive_ptr<TaskContext> for_release(ctx, /*add_ref=*/false);
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
