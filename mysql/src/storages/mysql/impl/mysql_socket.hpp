#pragma once

#include <engine/ev/watcher.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLSocket final {
 public:
  MySQLSocket(int fd, int mysql_events);
  ~MySQLSocket();

  bool ShouldWait() const;
  int Wait(engine::Deadline deadline);
  void SetEvents(int mysql_events);
  void SetFd(int fd);

  template <typename StartFn, typename ContFn>
  void RunToCompletion(StartFn&& start_fn, ContFn&& cont_fn,
                       engine::Deadline deadline);

  bool IsValid();

 private:
  static void WatcherCallback(struct ev_loop*, ev_io* watcher, int) noexcept;

  int fd_;

  std::atomic<int> mysql_events_to_wait_on_;
  engine::impl::FastPimplWaitListLight waiters_;
  engine::ev::Watcher<ev_io> watcher_;
};

template <typename StartFn, typename ContFn>
void MySQLSocket::RunToCompletion(StartFn&& start_fn, ContFn&& cont_fn,
                                  engine::Deadline deadline) {
  SetEvents(start_fn());
  while (ShouldWait()) {
    const auto mysql_events = Wait(deadline);
    SetEvents(cont_fn(mysql_events));
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
