#pragma once

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/fd_poller.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class Socket final {
 public:
  Socket(int fd, int mysql_events);
  ~Socket();

  void SetFd(int fd);

  template <typename StartFn, typename ContFn>
  void RunToCompletion(StartFn&& start_fn, ContFn&& cont_fn,
                       engine::Deadline deadline);

  bool IsValid() const;

 private:
  bool ShouldWait() const;
  int Wait(engine::Deadline deadline);
  void SetEvents(int mysql_events);

  void Reset();

  int fd_;
  engine::io::FdPoller poller_;

  int mysql_events_to_wait_on_;
};

template <typename StartFn, typename ContFn>
void Socket::RunToCompletion(StartFn&& start_fn, ContFn&& cont_fn,
                             engine::Deadline deadline) {
  SetEvents(start_fn());
  while (ShouldWait()) {
    const auto mysql_events = Wait(deadline);
    SetEvents(cont_fn(mysql_events));
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
