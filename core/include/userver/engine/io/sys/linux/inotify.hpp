#pragma once

#ifdef __linux__

/// @file userver/engine/io/sys/linux/inotify.hpp
/// @brief Linux-specific fs notification API

#include <sys/inotify.h>

#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

#include <userver/engine/io/fd_poller.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io::sys::linux {

/// @brief Notification event type
enum class EventType {
  kNone = 0,
  kAccess = IN_ACCESS,
  kAttribChanged = IN_ATTRIB,
  kCloseWrite = IN_CLOSE_WRITE,
  kCloseNoWrite = IN_CLOSE_NOWRITE,
  kCreate = IN_CREATE,
  kDelete = IN_DELETE,
  kDeleteSelf = IN_DELETE_SELF,
  kModify = IN_MODIFY,
  kMoveSelf = IN_MOVE_SELF,
  kMovedFrom = IN_MOVED_FROM,
  kMovedTo = IN_MOVED_TO,
  kOpen = IN_OPEN,

  kIsDir = IN_ISDIR,      // event occurred against dir
  kOnlyDir = IN_ONLYDIR,  // catch only dir events
};

std::string ToString(EventType);

/// @brief A set of notification event types
using EventTypeMask = utils::Flags<EventType>;

std::string ToString(EventTypeMask);

/// @brief Notification event
struct Event {
  std::string path;
  EventTypeMask mask;

  bool operator==(const Event& other) const {
    return path == other.path && mask == other.mask;
  }
};

logging::LogHelper& operator<<(logging::LogHelper& lh,
                               const Event& event) noexcept;

/// @brief File descriptor that allows one to monitor filesystem events, such as
/// file creation, modification, etc.
class Inotify final {
 public:
  Inotify();

  Inotify(const Inotify&) = delete;

  Inotify(Inotify&&) = delete;

  ~Inotify();

  Inotify& operator=(Inotify&&) = delete;

  Inotify& operator=(const Inotify&) = delete;

  /// Start to watch on the file path. Afterwards file events will be signaled
  /// through Poll() results.
  void AddWatch(const std::string& path, EventTypeMask flags);

  /// Stop watching file events previously registered through AddWatch().
  void RmWatch(const std::string& path);

  /// Read file events with event types previously registered through
  /// AddWatch(). Poll() blocks current coroutine until the event is obtained
  /// or the coroutine is cancelled.
  std::optional<Event> Poll(engine::Deadline deadline);

 private:
  void Dispatch();

  FdPoller fd_;
  std::queue<Event> pending_events_;
  std::unordered_map<std::string, int> path_to_wd_;
  std::unordered_map<int, std::string> wd_to_path_;
};

}  // namespace engine::io::sys::linux

USERVER_NAMESPACE_END

#endif  // __linux__
