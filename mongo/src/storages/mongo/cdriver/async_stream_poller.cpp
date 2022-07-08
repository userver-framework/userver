#include <storages/mongo/cdriver/async_stream_poller.hpp>

#include <poll.h>

#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

namespace {

void ConcurrentStop(int fd, std::mutex& mutex,
                    AsyncStreamPoller::WatchersMap& watchers) {
  std::unique_lock lock(mutex);
  const auto it = watchers.find(fd);
  if (it == watchers.end()) return;
  auto watcher = it->second.lock();
  lock.unlock();

  // After `watcher.Stop();` mutexes, watchers and AsyncStreamPoller may be
  // destroyed. But not before the Stop() call!
  if (watcher) watcher->Stop();
}

void StopWatchers(AsyncStreamPoller::WatchersMap& watchers) {
  for (auto& watcher_it : watchers) {
    auto watcher = watcher_it.second.lock();
    if (watcher) watcher->Stop();
  }
}

}  // namespace

AsyncStreamPoller::AsyncStreamPoller()
    : AsyncStreamPoller(engine::MpscQueue<Event>::Create()) {}

AsyncStreamPoller::~AsyncStreamPoller() {
  // ResetWatchers() acquires a lock and avoids race between watchers.find(fd);
  // and WatchersMap destruction.
  ResetWatchers();
}

AsyncStreamPoller::AsyncStreamPoller(
    const std::shared_ptr<engine::MpscQueue<Event>>& queue)
    : event_consumer_(queue->GetConsumer()),
      event_producer_(queue->GetProducer()) {}

AsyncStreamPoller::WatcherPtr AsyncStreamPoller::AddRead(int fd) {
  std::unique_lock lock(read_watchers_mutex_);
  auto& watcher_ptr = read_watchers_[fd];
  auto watcher = watcher_ptr.lock();
  if (!watcher) {
    watcher = std::make_shared<engine::ev::Watcher<ev_io>>(
        engine::current_task::GetEventThread(), this);
    watcher->Init(&IoEventCb, fd, EV_READ);
    watcher_ptr = watcher;
  }
  lock.unlock();
  watcher->StartAsync();
  return watcher;
}

AsyncStreamPoller::WatcherPtr AsyncStreamPoller::AddWrite(int fd) {
  std::unique_lock lock(write_watchers_mutex_);
  auto& watcher_ptr = write_watchers_[fd];
  auto watcher = watcher_ptr.lock();
  if (!watcher) {
    watcher = std::make_shared<engine::ev::Watcher<ev_io>>(
        engine::current_task::GetEventThread(), this);
    watcher->Init(&IoEventCb, fd, EV_WRITE);
    watcher_ptr = watcher;
  }
  lock.unlock();
  watcher->StartAsync();
  return watcher;
}

void AsyncStreamPoller::Reset() {
  ResetWatchers();

  Event buf;
  while (event_consumer_.PopNoblock(buf))
    ;  // discard
}

bool AsyncStreamPoller::NextEvent(Event& buf, engine::Deadline deadline) {
  return event_consumer_.Pop(buf, deadline);
}

bool AsyncStreamPoller::NextEventNoblock(Event& buf) {
  return event_consumer_.PopNoblock(buf);
}

void AsyncStreamPoller::StopRead(int fd) {
  ConcurrentStop(fd, read_watchers_mutex_, read_watchers_);
}

void AsyncStreamPoller::StopWrite(int fd) {
  ConcurrentStop(fd, write_watchers_mutex_, write_watchers_);
}

void AsyncStreamPoller::ResetWatchers() {
  // destroy watchers without lock to avoid deadlocking with callback
  WatchersMap watchers_buffer;
  {
    std::lock_guard read_lock(read_watchers_mutex_);
    read_watchers_.swap(watchers_buffer);
  }
  StopWatchers(watchers_buffer);
  watchers_buffer.clear();
  {
    std::lock_guard write_lock(write_watchers_mutex_);
    write_watchers_.swap(watchers_buffer);
  }
  StopWatchers(watchers_buffer);
  watchers_buffer.clear();
}

void AsyncStreamPoller::IoEventCb(struct ev_loop*, ev_io* watcher,
                                  int revents) noexcept {
  UASSERT(watcher->active);
  UASSERT((watcher->events & ~(EV_READ | EV_WRITE)) == 0);

  auto* poller = static_cast<AsyncStreamPoller*>(watcher->data);
  auto event_type = Event::kError;
  if (revents & EV_ERROR) {
    // highest priority, can be mixed with dummy events
    event_type = Event::kError;
  } else if (revents & EV_READ) {
    event_type = Event::kRead;
  } else if (revents & EV_WRITE) {
    event_type = Event::kWrite;
  }
  // NOTE: it might be better to poll() here to get POLLERR/POLLHUP as well
  [[maybe_unused]] bool is_sent =
      poller->event_producer_.PushNoblock({watcher->fd, event_type});
  UASSERT(is_sent);

  if (watcher->events & EV_READ) {
    UASSERT(!(watcher->events & EV_WRITE));
    poller->StopRead(watcher->fd);
  } else if (watcher->events & EV_WRITE) {
    poller->StopWrite(watcher->fd);
  } else {
    LOG_LIMITED_ERROR() << "Watcher is neither read nor write watcher, events="
                        << watcher->events;
    UASSERT_MSG(false, "Watcher is neither read nor write watcher");
  }
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
