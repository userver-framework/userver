#pragma once

#include <mutex>
#include <unordered_map>

#include <engine/deadline.hpp>
#include <engine/io/common.hpp>
#include <engine/mpsc_queue.hpp>

#include <engine/ev/watcher.hpp>

namespace engine::io {

// Not thread-safe
// Reports HUP as readiness (libev limitation)
class Poller {
 public:
  struct Event {
    int fd{kInvalidFd};
    enum { kError = -1, kRead, kWrite } type{kError};
  };

  Poller();

  Poller(const Poller&) = delete;
  Poller(Poller&&) = delete;

  void Reset();

  void AddRead(int fd);
  void AddWrite(int fd);

  bool NextEvent(Event&, Deadline);
  bool NextEventNoblock(Event&);

 private:
  explicit Poller(const std::shared_ptr<MpscQueue<Event>>&);

  void StopRead(int fd);
  void StopWrite(int fd);

  static void IoEventCb(struct ev_loop*, ev_io*, int) noexcept;

  MpscQueue<Event>::Consumer event_consumer_;
  MpscQueue<Event>::Producer event_producer_;
  std::mutex read_watchers_mutex_;
  std::unordered_map<int, ev::Watcher<ev_io>> read_watchers_;
  std::mutex write_watchers_mutex_;
  std::unordered_map<int, ev::Watcher<ev_io>> write_watchers_;
};

}  // namespace engine::io
