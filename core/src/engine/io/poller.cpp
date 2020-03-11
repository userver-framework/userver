#include <engine/io/poller.hpp>

#include <poll.h>

#include <engine/task/task.hpp>
#include <utils/assert.hpp>

namespace engine::io {

namespace {

void ConcurrentStop(int fd, std::mutex& mutex, Poller::WatchersMap& watchers) {
  std::unique_lock lock(mutex);
  auto watcher = watchers.extract(fd);
  if (!watcher) return;
  lock.unlock();

  watcher.mapped().Stop();

  // Concurrent invocation of Poller::Reset() may have finished at this point.
  // In that case `watchers` will have either:
  // 1) a useless watcher that isn't watched and removed on next Poller::Reset()
  // 2) watcher for an closed and opened `fd` (in that case libev automatically
  // adds fd to epoll watch list )

  lock.lock();
  watchers.insert(std::move(watcher));
  lock.unlock();

  // destroying watcher
}

}  // namespace

Poller::Poller() : Poller(MpscQueue<Event>::Create()) {}

Poller::Poller(const std::shared_ptr<MpscQueue<Event>>& queue)
    : event_consumer_(queue->GetConsumer()),
      event_producer_(queue->GetProducer()) {}

void Poller::AddRead(int fd) {
  std::unique_lock lock(read_watchers_mutex_);
  auto [it, was_inserted] = read_watchers_.emplace(
      std::piecewise_construct, std::make_tuple(fd),
      std::make_tuple(std::ref(current_task::GetEventThread()), this));
  auto& watcher = it->second;
  lock.unlock();
  if (was_inserted) {
    watcher.Init(&IoEventCb, fd, EV_READ);
  }
  watcher.StartAsync();
}

void Poller::AddWrite(int fd) {
  std::unique_lock lock(write_watchers_mutex_);
  auto [it, was_inserted] = write_watchers_.emplace(
      std::piecewise_construct, std::make_tuple(fd),
      std::make_tuple(std::ref(current_task::GetEventThread()), this));
  auto& watcher = it->second;
  lock.unlock();
  if (was_inserted) {
    watcher.Init(&IoEventCb, fd, EV_WRITE);
  }
  watcher.StartAsync();
}

void Poller::Reset() {
  // destroy watchers without lock to avoid deadlocking with callback
  std::unordered_map<int, ev::Watcher<ev_io>> watchers_buffer;
  {
    std::lock_guard read_lock(read_watchers_mutex_);
    read_watchers_.swap(watchers_buffer);
  }
  watchers_buffer.clear();
  {
    std::lock_guard write_lock(write_watchers_mutex_);
    write_watchers_.swap(watchers_buffer);
  }
  watchers_buffer.clear();

  Event buf;
  while (event_consumer_.PopNoblock(buf))
    ;  // discard
}

bool Poller::NextEvent(Event& buf, Deadline deadline) {
  return event_consumer_.Pop(buf, deadline);
}

bool Poller::NextEventNoblock(Event& buf) {
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  return event_consumer_.PopNoblock(buf);
}

void Poller::StopRead(int fd) {
  ConcurrentStop(fd, read_watchers_mutex_, read_watchers_);
}

void Poller::StopWrite(int fd) {
  ConcurrentStop(fd, write_watchers_mutex_, write_watchers_);
}

void Poller::IoEventCb(struct ev_loop*, ev_io* watcher, int revents) noexcept {
  auto* poller = static_cast<Poller*>(watcher->data);
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
    LOG_ERROR() << "Watcher is neither read nor write watcher, events="
                << watcher->events;
    UASSERT_MSG(false, "Watcher is neither read nor write watcher");
  }
}

}  // namespace engine::io
