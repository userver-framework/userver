#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/mpsc_queue.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// @brief I/O event monitor.
/// @warning Generally not thread-safe, awaited events should not be changed
/// during active waiting.
/// @note Reports HUP as readiness.
class Poller final {
 public:
  /// I/O event.
  struct Event {
    /// I/O event type.
    enum Type {
      kNone = 0,          ///< No active event (or interruption)
      kRead = (1 << 0),   ///< File descriptor is ready for reading
      kWrite = (1 << 1),  ///< File descriptor is ready for writing
      kError =
          (1 << 2),  ///< File descriptor is in error state (always awaited)
    };

    /// File descriptor responsible for the event
    int fd{kInvalidFd};
    /// Triggered event types
    utils::Flags<Type> type{kNone};
    /// Event epoch, for internal use
    size_t epoch{0};
  };

  /// Event retrieval status
  enum class [[nodiscard]] Status {
    kSuccess,    ///< Received an event
    kInterrupt,  ///< Received an interrupt request
    kNoEvents,   ///< No new events available or task has been cancelled
  };

  Poller();

  Poller(const Poller&) = delete;
  Poller(Poller&&) = delete;

  /// Updates a set of events to be monitored for the file descriptor.
  ///
  /// At most one set of events is reported for every `Add` invocation.
  ///
  /// @note Event::Type::kError is implicit when any other event type is
  /// specified.
  void Add(int fd, utils::Flags<Event::Type> events);

  /// Disables event monitoring on a specific file descriptor.
  ///
  /// This function must be called before closing the socket.
  void Remove(int fd);

  /// Waits for the next event and stores it at the provided structure.
  Status NextEvent(Event&, Deadline);

  /// Outputs the next event if immediately available.
  Status NextEventNoblock(Event&);

  /// Emits an event for an invalid fd with empty event types set.
  void Interrupt();

 private:
  explicit Poller(const std::shared_ptr<MpscQueue<Event>>&);

  struct IoWatcher {
    explicit IoWatcher(Poller&);

    IoWatcher(const IoWatcher&) = delete;
    IoWatcher(IoWatcher&&) = delete;

    Poller& poller;
    std::atomic<size_t> coro_epoch;
    size_t ev_epoch{0};
    ev::Watcher<ev_io> ev_watcher;
    utils::AtomicFlags<Event::Type> awaited_events;
  };

  template <typename EventSource>
  Status EventsFilter(EventSource, Event&);

  static void IoEventCb(struct ev_loop*, ev_io*, int) noexcept;

  MpscQueue<Event>::Consumer event_consumer_;
  MpscQueue<Event>::Producer event_producer_;
  std::unordered_map<int, IoWatcher> watchers_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
