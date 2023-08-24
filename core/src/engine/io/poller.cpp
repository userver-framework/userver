#include <engine/io/poller.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

namespace {

int ToEvEvents(utils::Flags<Poller::Event::Type> events) {
  int ev_events = 0;
  if (events & Poller::Event::kRead) ev_events |= EV_READ;
  if (events & Poller::Event::kWrite) ev_events |= EV_WRITE;
  return ev_events;
}

utils::Flags<Poller::Event::Type> FromEvEvents(int ev_events) {
  if (ev_events & EV_ERROR) {
    // highest priority, can be mixed with dummy events
    return Poller::Event::kError;
  }

  utils::Flags<Poller::Event::Type> events;
  if (ev_events & EV_READ) events |= Poller::Event::kRead;
  if (ev_events & EV_WRITE) events |= Poller::Event::kWrite;
  return events;
}

}  // namespace

Poller::Poller()
    : Poller(USERVER_NAMESPACE::concurrent::MpscQueue<Event>::Create()) {}

Poller::Poller(
    const std::shared_ptr<USERVER_NAMESPACE::concurrent::MpscQueue<Event>>&
        queue)
    : event_consumer_(queue->GetConsumer()),
      event_producer_(queue->GetProducer()) {}

void Poller::Add(int fd, utils::Flags<Event::Type> events) {
  auto& watcher = watchers_.emplace(fd, *this).first->second;

  const auto old_events = watcher.awaited_events.Exchange(events);
  if (old_events == events) return;

  ++watcher.coro_epoch;
  watcher.ev_watcher.RunInBoundEvLoopAsync([&watcher, fd,
                                            should_stop = !!old_events,
                                            ev_events = ToEvEvents(events)] {
    // watcher lifetime is guarded by ev_watcher dtor
    if (should_stop) watcher.ev_watcher.Stop();
    ++watcher.ev_epoch;
    if (ev_events && watcher.ev_epoch == watcher.coro_epoch) {
      watcher.ev_watcher.Set(fd, ev_events);
      watcher.ev_watcher.Start();
    }
  });
}

void Poller::Remove(int fd) {
  auto watcher_it = watchers_.find(fd);
  UASSERT_MSG(watcher_it != watchers_.end(),
              "Request for removal of an unknown fd from poller");
  if (watcher_it == watchers_.end()) return;
  auto& watcher = watcher_it->second;

  watcher.awaited_events = Event::kNone;

  // At this point Poller::IoEventCb may be calling Stop() on watcher,
  // Poller::EventsFilter may have been already called and
  // awaited_events == Event::kNone.
  //
  // Have to call Stop() for watcher to avoid early return from this function,
  // close of the fd and Watcher<ev_io>::StopImpl() reporting bad fd.
  //
  // Watching a bad fd results in EV_ERROR, which is an application bug.

  ++watcher.coro_epoch;
  watcher.ev_watcher.RunInBoundEvLoopSync([&watcher] {
    watcher.ev_watcher.Stop();
    ++watcher.ev_epoch;
  });
}

Poller::Status Poller::NextEvent(Event& buf, Deadline deadline) {
  return EventsFilter(
      [this, deadline](Event& buf_) {
        return event_consumer_.Pop(buf_, deadline);
      },
      buf);
}

Poller::Status Poller::NextEventNoblock(Event& buf) {
  return EventsFilter(
      [this](Event& buf_) { return event_consumer_.PopNoblock(buf_); }, buf);
}

void Poller::Interrupt() {
  [[maybe_unused]] bool is_sent = event_producer_.PushNoblock({});
  UASSERT(is_sent);
}

template <typename EventSource>
Poller::Status Poller::EventsFilter(EventSource get_event, Event& buf) {
  bool has_event = get_event(buf);
  while (has_event) {
    if (buf.fd == kInvalidFd) return Status::kInterrupt;

    const auto it = watchers_.find(buf.fd);
    UASSERT(it != watchers_.end());
    buf.type &= Event::kError | it->second.awaited_events;
    if (buf.epoch == it->second.coro_epoch && buf.type) {
      it->second.awaited_events = Event::kNone;
      break;
    }
    has_event = get_event(buf);
  }
  return has_event ? Status::kSuccess : Status::kNoEvents;
}

void Poller::IoEventCb(struct ev_loop*, ev_io* watcher, int revents) noexcept {
  UASSERT(watcher->active);
  UASSERT((watcher->events & ~(EV_READ | EV_WRITE)) == 0);

  auto* watcher_meta = static_cast<IoWatcher*>(watcher->data);
  UASSERT(watcher_meta);

  const auto ev_epoch = watcher_meta->ev_epoch;
  if (watcher_meta->coro_epoch != ev_epoch) return;

  // NOTE: it might be better to poll() here to get POLLERR/POLLHUP as well
  [[maybe_unused]] bool is_sent =
      watcher_meta->poller.event_producer_.PushNoblock(
          {watcher->fd, FromEvEvents(revents), ev_epoch});
  UASSERT(is_sent);

  watcher_meta->ev_watcher.Stop();
}

Poller::IoWatcher::IoWatcher(Poller& owner)
    : poller(owner),
      coro_epoch{0},
      ev_watcher(engine::current_task::GetEventThread(), this) {
  ev_watcher.Init(&IoEventCb);
}

}  // namespace engine::io

USERVER_NAMESPACE_END
