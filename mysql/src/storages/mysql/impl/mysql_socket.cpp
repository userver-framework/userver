#include <storages/mysql/impl/mysql_socket.hpp>

#include <utility>

#include <mysql/mysql.h>

#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLSocket::MySQLSocket(int fd, int mysql_events)
    : fd_{fd},
      mysql_events_to_wait_on_{mysql_events},
      watcher_{engine::current_task::GetEventThread(), this} {
  watcher_.Init(&WatcherCallback);
}

MySQLSocket::~MySQLSocket() { watcher_.Stop(); }

bool MySQLSocket::ShouldWait() const { return mysql_events_to_wait_on_ != 0; }

int MySQLSocket::Wait(engine::Deadline deadline) {
  UASSERT(mysql_events_to_wait_on_ != 0);

  int ev_events = 0;
  if (mysql_events_to_wait_on_ & MYSQL_WAIT_READ) ev_events |= EV_READ;
  if (mysql_events_to_wait_on_ & MYSQL_WAIT_WRITE) ev_events |= EV_WRITE;

  watcher_.RunInBoundEvLoopSync([this, ev_events] {
    watcher_.Stop();
    watcher_ready_event_.Reset();
    watcher_.Set(fd_, ev_events);
    watcher_.Start();
  });

  if (!watcher_ready_event_.WaitForEventUntil(deadline)) {
    throw std::runtime_error{"Wait deadline, idk"};
  }
  return std::exchange(mysql_events_to_wait_on_, 0);
}

void MySQLSocket::SetEvents(int mysql_events) {
  UASSERT(mysql_events_to_wait_on_ == 0);

  watcher_.RunInBoundEvLoopSync(
      [this, mysql_events] { mysql_events_to_wait_on_ = mysql_events; });
}

void MySQLSocket::WatcherCallback(struct ev_loop*, ev_io* watcher,
                                  int) noexcept {
  auto* self = static_cast<MySQLSocket*>(watcher->data);

  int mysql_events = 0;
  if (watcher->events & EV_READ) mysql_events |= MYSQL_WAIT_READ;
  if (watcher->events & EV_WRITE) mysql_events |= MYSQL_WAIT_WRITE;

  self->watcher_.Stop();

  self->mysql_events_to_wait_on_ = mysql_events;
  self->watcher_ready_event_.Send();
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
