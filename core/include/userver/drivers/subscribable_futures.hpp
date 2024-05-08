#pragma once

/// @file userver/drivers/subscribable_futures.hpp
/// @brief @copybrief drivers::SubscribableFutureWrapper

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <userver/engine/deadline.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/make_intrusive_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace drivers {

namespace impl {

struct EventHolder final : boost::intrusive_ref_counter<EventHolder> {
  engine::SingleConsumerEvent event{engine::SingleConsumerEvent::NoAutoReset{}};
};

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief An adaptor for working with certain external futures.
///
/// The `SubscribableFuture` must have a `Subscribe(Callback)` method that calls
/// the callback when the future is ready. If the future is already ready, then
/// `Subscribe` must call the callback immediately. If the promise is dropped
/// and will never be fulfilled properly, then the `SubscribableFuture` should
/// call the callback anyway.
///
/// @see drivers::WaitForSubscribableFuture
/// @see drivers::TryWaitForSubscribableFuture
template <typename SubscribableFuture>
class SubscribableFutureWrapper final {
 public:
  explicit SubscribableFutureWrapper(SubscribableFuture&& future)
      : original_future_(static_cast<SubscribableFuture&&>(future)),
        event_holder_(utils::make_intrusive_ptr<impl::EventHolder>()) {
    original_future_.Subscribe(
        [event_holder = event_holder_](auto&) { event_holder->event.Send(); });
  }

  /// @returns the original future
  SubscribableFuture& GetFuture() { return original_future_; }

  /// @brief Wait for the future. The result can be retrieved from the original
  /// future using GetFuture once ready.
  /// @throws engine::WaitInterruptedException on task cancellation
  void Wait() {
    if (TryWaitUntil(engine::Deadline{}) != engine::FutureStatus::kReady) {
      throw engine::WaitInterruptedException(
          engine::current_task::CancellationReason());
    }
  }

  /// @brief Wait for the future. The result can be retrieved from the original
  /// future using GetFuture once ready.
  /// @returns an error code if deadline is exceeded or task is cancelled
  [[nodiscard]] engine::FutureStatus TryWaitUntil(engine::Deadline deadline) {
    if (event_holder_->event.WaitForEventUntil(deadline)) {
      return engine::FutureStatus::kReady;
    }
    return engine::current_task::ShouldCancel()
               ? engine::FutureStatus::kCancelled
               : engine::FutureStatus::kTimeout;
  }

 private:
  SubscribableFuture original_future_;
  boost::intrusive_ptr<impl::EventHolder> event_holder_;
};

/// @ingroup userver_concurrency
///
/// @brief Waits on the given future as described
/// on drivers::SubscribableFutureWrapper.
///
/// The result can be retrieved from the original future once ready.
///
/// @throws engine::WaitInterruptedException on task cancellation
template <typename SubscribableFuture>
void WaitForSubscribableFuture(SubscribableFuture&& future) {
  SubscribableFutureWrapper<SubscribableFuture&>{future}.Wait();
}

/// @ingroup userver_concurrency
/// @overload
/// @returns an error code if deadline is exceeded or task is cancelled
///
/// @warning Repeatedly waiting again after `deadline` expiration leads to a
/// memory leak, use drivers::SubscribableFutureWrapper instead.
template <typename SubscribableFuture>
[[nodiscard]] engine::FutureStatus TryWaitForSubscribableFuture(
    SubscribableFuture&& future, engine::Deadline deadline) {
  return SubscribableFutureWrapper<SubscribableFuture&>{future}.TryWaitUntil(
      deadline);
}

}  // namespace drivers

USERVER_NAMESPACE_END
