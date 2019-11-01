#include <engine/io/poller.hpp>

#include <poll.h>

#include <engine/task/task.hpp>
#include <utils/assert.hpp>

namespace engine::io {

Poller::Poller() : Poller(MpscQueue<Event>::Create()) {}

Poller::Poller(const std::shared_ptr<MpscQueue<Event>>& queue)
    : event_consumer_(queue->GetConsumer()),
      event_producer_(queue->GetProducer()) {}

void Poller::AddRead(int fd) {
  auto [it, was_inserted] = read_watchers_.emplace(
      std::piecewise_construct, std::make_tuple(fd),
      std::make_tuple(std::ref(current_task::GetEventThread()), this));
  if (was_inserted) {
    it->second.Init(&IoEventCb, fd, EV_READ);
  }
  it->second.StartAsync();
}

void Poller::AddWrite(int fd) {
  auto [it, was_inserted] = write_watchers_.emplace(
      std::piecewise_construct, std::make_tuple(fd),
      std::make_tuple(std::ref(current_task::GetEventThread()), this));
  if (was_inserted) {
    it->second.Init(&IoEventCb, fd, EV_WRITE);
  }
  it->second.StartAsync();
}

void Poller::Reset() {
  read_watchers_.clear();
  write_watchers_.clear();

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
  auto it = read_watchers_.find(fd);
  if (it != read_watchers_.end()) {
    it->second.Stop();
  }
}

void Poller::StopWrite(int fd) {
  auto it = write_watchers_.find(fd);
  if (it != write_watchers_.end()) {
    it->second.Stop();
  }
}

void Poller::IoEventCb(struct ev_loop*, ev_io* watcher, int revents) noexcept {
  auto* poller = static_cast<Poller*>(watcher->data);
  auto event_type = Event::kError;
  if (revents == EV_READ) {
    event_type = Event::kRead;
  } else if (revents == EV_WRITE) {
    event_type = Event::kWrite;
  }
  // NOTE: it might be better to poll() here to get POLLERR/POLLHUP as well
  [[maybe_unused]] bool is_sent =
      poller->event_producer_.PushNoblock({watcher->fd, event_type});
  UASSERT(is_sent);

  if (event_type == Event::kRead || event_type == Event::kError) {
    poller->StopRead(watcher->fd);
  }
  if (event_type == Event::kWrite || event_type == Event::kError) {
    poller->StopWrite(watcher->fd);
  }
}

}  // namespace engine::io
