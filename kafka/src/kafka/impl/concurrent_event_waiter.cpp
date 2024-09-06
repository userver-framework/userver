#include <kafka/impl/concurrent_event_waiter.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

void ConcurrentEventWaiters::PushWaiter(EventWaiter& waiter) const {
  auto locked_waiters = waiters_.Lock();
  locked_waiters->push_back(waiter);
}

void ConcurrentEventWaiters::PopWaiter(EventWaiter& waiter) const {
  auto locked_waiters = waiters_.Lock();
  if (!waiter.event.IsReady()) {
    auto waiter_it = locked_waiters->s_iterator_to(waiter);
    locked_waiters->erase(waiter_it);
  }
}

void ConcurrentEventWaiters::PopAndWakeupOne() const {
  auto locked_waiters = waiters_.Lock();
  if (locked_waiters->empty()) {
    LOG_DEBUG() << "No waiters waked up";
    return;
  }
  auto& waiter = locked_waiters->front();
  if (!waiter.event.IsReady()) {
    locked_waiters->pop_front();
    waiter.event.Send();
  }
}

void ConcurrentEventWaiters::WakeupAll() const {
  auto locked_waiters = waiters_.Lock();
  for (auto& waiter : *locked_waiters) {
    if (!waiter.event.IsReady()) {
      waiter.event.Send();
    }
  }
  locked_waiters->clear();
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
