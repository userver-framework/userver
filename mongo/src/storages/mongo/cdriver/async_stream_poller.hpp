#pragma once

#include <mutex>
#include <unordered_map>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>

#include <engine/ev/watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

// AsyncStream-specific implementation of global poller object
// Not thread-safe
// Reports HUP as readiness (libev limitation)
class AsyncStreamPoller final {
 public:
  using WatcherPtr = std::shared_ptr<engine::ev::Watcher<ev_io>>;
  using WatcherWeakPtr = std::weak_ptr<engine::ev::Watcher<ev_io>>;
  using WatchersMap = std::unordered_map<int, WatcherWeakPtr>;

  struct Event {
    int fd{engine::io::kInvalidFd};
    enum { kError = -1, kRead, kWrite } type{kError};
  };

  AsyncStreamPoller();
  ~AsyncStreamPoller();

  AsyncStreamPoller(const AsyncStreamPoller&) = delete;
  AsyncStreamPoller(AsyncStreamPoller&&) = delete;

  void Reset();

  [[nodiscard]] WatcherPtr AddRead(int fd);
  [[nodiscard]] WatcherPtr AddWrite(int fd);

  bool NextEvent(Event&, engine::Deadline);
  bool NextEventNoblock(Event&);

 private:
  explicit AsyncStreamPoller(
      const std::shared_ptr<concurrent::MpscQueue<Event>>&);

  void StopRead(int fd);
  void StopWrite(int fd);
  void ResetWatchers();

  static void IoEventCb(struct ev_loop*, ev_io*, int) noexcept;

  concurrent::MpscQueue<Event>::Consumer event_consumer_;
  concurrent::MpscQueue<Event>::Producer event_producer_;
  std::mutex read_watchers_mutex_;
  WatchersMap read_watchers_;
  std::mutex write_watchers_mutex_;
  WatchersMap write_watchers_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
