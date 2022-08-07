#include "wait_list_light.hpp"

#include <cstddef>

#include <boost/atomic/atomic.hpp>

#include <engine/task/task_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

struct alignas(sizeof(void*) * 2) Waiter {
  TaskContext* context{nullptr};
  std::uintptr_t epoch{0};
};

static_assert(boost::atomic<Waiter>::is_always_lock_free);

}  // namespace

struct WaitListLight::Impl final {
  // std::atomic refuses to produce lock-free instructions on some compilers.
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522
  boost::atomic<Waiter> waiter;
};

WaitListLight::WaitListLight() noexcept : impl_() {}

WaitListLight::~WaitListLight() {
  UASSERT_MSG(impl_->waiter.load().context == nullptr,
              "Someone is waiting on WaitListLight while it's being destroyed");
}

void WaitListLight::Append(boost::intrusive_ptr<TaskContext> context) noexcept {
  UASSERT(context);
  UASSERT(context->IsCurrent());
  UASSERT_MSG(!impl_->waiter.load(boost::memory_order_relaxed).context,
              "Another coroutine is already waiting on this WaitListLight");
  LOG_TRACE() << "Appending, use_count=" << context->use_count();

  const std::uintptr_t epoch{utils::UnderlyingValue(context->GetEpoch())};

  // Keep a reference logically stored in the WaitListLight to ensure that
  // WakeupOne can complete safely in parallel with the waiting task being
  // cancelled, Remove-d and stopped.
  auto* const ctx = context.detach();

  impl_->waiter.store({ctx, epoch}, boost::memory_order_seq_cst);
}

void WaitListLight::WakeupOne() {
  const auto waiter = impl_->waiter.exchange({});
  const boost::intrusive_ptr<TaskContext> context{waiter.context,
                                                  /*add_ref=*/false};
  if (!context) return;

  const auto epoch = static_cast<SleepState::Epoch>(waiter.epoch);

  LOG_TRACE() << "Waking up, use_count=" << context->use_count();
  context->Wakeup(TaskContext::WakeupSource::kWaitList, epoch);
}

void WaitListLight::Remove(TaskContext& context) noexcept {
  UASSERT(context.IsCurrent());

  const auto waiter = impl_->waiter.exchange({});
  const boost::intrusive_ptr<TaskContext> removed_context{waiter.context,
                                                          /*add_ref=*/false};

  if (!removed_context) return;
  UASSERT_MSG(removed_context == &context,
              "Attempting to wait in a single WaitListLight from multiple "
              "coroutines");

  LOG_TRACE() << "Remove";
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
