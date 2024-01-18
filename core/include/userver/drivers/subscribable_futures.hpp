#pragma once

/// @file userver/drivers/subscribable_futures.hpp
/// @brief @copybrief drivers::WaitForSubscribableFuture

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
/// @throws engine::WaitInterruptedException on task cancellation
template <typename SubscribableFuture>
void WaitForSubscribableFuture(SubscribableFuture&& future);

/// @overload
/// @ingroup userver_concurrency
///
/// @brief Same as drivers::WaitForSubscribableFuture, but returns an error code
/// instead of throwing.
template <typename SubscribableFuture>
[[nodiscard]] engine::FutureStatus TryWaitForSubscribableFuture(
    SubscribableFuture&& future, engine::Deadline deadline);

namespace impl {

struct EventHolder final : boost::intrusive_ref_counter<EventHolder> {
  engine::SingleConsumerEvent event{engine::SingleConsumerEvent::NoAutoReset{}};
};

}  // namespace impl

template <typename SubscribableFuture>
void WaitForSubscribableFuture(SubscribableFuture&& future) {
  if (drivers::TryWaitForSubscribableFuture(future, engine::Deadline{}) !=
      engine::FutureStatus::kReady) {
    throw engine::WaitInterruptedException(
        engine::current_task::CancellationReason());
  }
}

template <typename SubscribableFuture>
[[nodiscard]] engine::FutureStatus TryWaitForSubscribableFuture(
    SubscribableFuture&& future, engine::Deadline deadline) {
  auto event_holder = utils::make_intrusive_ptr<impl::EventHolder>();
  future.Subscribe([event_holder](auto&) { event_holder->event.Send(); });
  if (event_holder->event.WaitForEventUntil(deadline)) {
    return engine::FutureStatus::kReady;
  }
  return engine::current_task::ShouldCancel() ? engine::FutureStatus::kCancelled
                                              : engine::FutureStatus::kTimeout;
}

}  // namespace drivers

USERVER_NAMESPACE_END
