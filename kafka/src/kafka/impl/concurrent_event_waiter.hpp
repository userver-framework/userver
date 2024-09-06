#pragma once

#include <mutex>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>
#include <boost/intrusive/options.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/single_use_event.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

struct EventWaiter final
    : public boost::intrusive::list_base_hook<utils::impl::IntrusiveLinkMode> {
  engine::SingleUseEvent event;
};

using EventWaiters =
    boost::intrusive::list<EventWaiter,
                           boost::intrusive::constant_time_size<false>>;

class ConcurrentEventWaiters {
 public:
  /// @brief Adds waiter to waiting list.
  void PushWaiter(EventWaiter& waiter) const;

  /// @brief Removes waiter from waiting list if not yet removed.
  /// Waiter is in list <=> !waiter.event.IsReady()
  void PopWaiter(EventWaiter& waiter) const;

  /// @brief Removes one waiter from the list and wakes it up.
  void PopAndWakeupOne() const;

  /// @brief Wakes all waiters and clears the list.
  void WakeupAll() const;

 private:
  /// std::mutex is used because (i) it is faster for small critical sections
  /// (like std::list insert and erase), (ii) it can be used in non-coroutine
  /// environment
  mutable concurrent::Variable<EventWaiters, std::mutex> waiters_;
};

}  // namespace kafka::impl

USERVER_NAMESPACE_END
