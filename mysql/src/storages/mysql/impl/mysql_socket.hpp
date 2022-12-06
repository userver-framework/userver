#pragma once

#include <engine/ev/watcher.hpp>
#include <userver/engine/single_consumer_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLSocket final {
 public:
  MySQLSocket(int fd, int mysql_events);
  ~MySQLSocket();

  bool ShouldWait() const;
  int Wait(engine::Deadline deadline);
  void SetEvents(int mysql_events);

 private:
  static void WatcherCallback(struct ev_loop*, ev_io* watcher, int) noexcept;

  const int fd_;

  int mysql_events_to_wait_on_;

  engine::SingleConsumerEvent watcher_ready_event_;
  engine::ev::Watcher<ev_io> watcher_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
