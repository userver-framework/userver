#include <storages/mysql/impl/mysql_socket.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

class SocketWaitStrategy final : public engine::impl::WaitStrategy {
 public:
  SocketWaitStrategy(engine::Deadline deadline,
                     engine::impl::WaitListLight& waiters,
                     engine::ev::Watcher<ev_io>& watcher,
                     engine::impl::TaskContext& current)
      : engine::impl::WaitStrategy(deadline),
        waiters_{waiters},
        watcher_{watcher},
        current_{current} {}

  void SetupWakeups() override {
    waiters_.Append(&current_);
    watcher_.StartAsync();
  }

  void DisableWakeups() override {
    waiters_.Remove(current_);
    watcher_.StopAsync();
  }

 private:
  engine::impl::WaitListLight& waiters_;
  engine::ev::Watcher<ev_io>& watcher_;
  engine::impl::TaskContext& current_;
};

engine::impl::TaskContext::WakeupSource DoWait(
    engine::Deadline deadline, engine::impl::WaitListLight& waiters,
    engine::ev::Watcher<ev_io>& watcher) {
  auto& current = engine::current_task::GetCurrentTaskContext();
  if (current.ShouldCancel()) {
    return engine::impl::TaskContext::WakeupSource::kCancelRequest;
  }

  SocketWaitStrategy wait_strategy{deadline, waiters, watcher, current};
  auto ret = current.Sleep(wait_strategy);

  watcher.Stop();
  return ret;
}

}  // namespace

MySQLSocket::MySQLSocket(int fd, int mysql_events)
    : fd_{fd},
      mysql_events_to_wait_on_{mysql_events},
      watcher_{engine::current_task::GetEventThread(), this} {
  watcher_.Init(&WatcherCallback);
}

MySQLSocket::~MySQLSocket() { watcher_.Stop(); }

bool MySQLSocket::ShouldWait() const {
  return mysql_events_to_wait_on_.load() != 0;
}

int MySQLSocket::Wait(engine::Deadline deadline) {
  UASSERT(mysql_events_to_wait_on_.load() != 0);

  int ev_events = 0;
  const auto mysql_events = mysql_events_to_wait_on_.load();
  if (mysql_events & MYSQL_WAIT_READ) ev_events |= EV_READ;
  if (mysql_events & MYSQL_WAIT_WRITE) ev_events |= EV_WRITE;

  watcher_.RunInBoundEvLoopAsync(
      [this, ev_events] { watcher_.Set(fd_, ev_events); });

  using WakeupSource = engine::impl::TaskContext::WakeupSource;
  auto ret = DoWait(deadline, *waiters_, watcher_);
  if (ret != WakeupSource::kWaitList) {
    if (ret == WakeupSource::kCancelRequest) {
      // TODO : CancellationPoint()?
      throw std::runtime_error{"Wait cancelled"};
    } else if (ret == WakeupSource::kDeadlineTimer) {
      throw std::runtime_error{"Wait timed out"};
    }
  }

  return mysql_events_to_wait_on_.exchange(0);
}

void MySQLSocket::SetEvents(int mysql_events) {
  // TODO : think about this one, can fail if `Wait` timed out/was cancelled
  UASSERT(mysql_events_to_wait_on_.load() == 0);

  mysql_events_to_wait_on_.store(mysql_events);
}

void MySQLSocket::WatcherCallback(struct ev_loop*, ev_io* watcher,
                                  int) noexcept {
  auto* self = static_cast<MySQLSocket*>(watcher->data);

  int mysql_events = 0;
  if (watcher->events & EV_READ) mysql_events |= MYSQL_WAIT_READ;
  if (watcher->events & EV_WRITE) mysql_events |= MYSQL_WAIT_WRITE;

  self->watcher_.Stop();

  self->mysql_events_to_wait_on_.store(mysql_events);
  self->waiters_->WakeupOne();
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
