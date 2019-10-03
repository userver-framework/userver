#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher.hpp>
#include <engine/task/task_processor.hpp>

namespace engine {
namespace ev {

/* Async fd watcher that calls callbacks in IO thread */
class IoWatcher {
 public:
  explicit IoWatcher(ThreadControl& thread_control);
  IoWatcher(const IoWatcher&) = delete;
  // NOLINTNEXTLINE(bugprone-exception-escape)
  virtual ~IoWatcher();

  /* Set fd.  After the call IoWatcher owns the fd and eventually calls close().
   */
  void SetFd(int fd);

  bool HasFd() const;
  int GetFd() const;

  /* Return current fd without calling close(2) on it; after the call IoWatcher
   * doesn't own fd. */
  int Release();

  using Callback = std::function<void(std::error_code)>;
  void ReadAsync(Callback cb);

  void WriteAsync(Callback cb);

  void Cancel();
  void CloseFd();

 private:
  void Start();
  void Stop();
  void Loop();

  static void OnEventRead(struct ev_loop* loop, ev_io* io, int events) noexcept;
  static void OnEventWrite(struct ev_loop* loop, ev_io* io,
                           int events) noexcept;

  void CallReadCb(std::error_code ec);
  void CallWriteCb(std::error_code ec);

  void CancelSingle(Watcher<ev_io>& watcher, Callback& cb);

 private:
  int fd_;
  Watcher<ev_io> watcher_read_, watcher_write_;

  std::mutex mutex_;  // guards cb_*
  Callback cb_read_, cb_write_;
};

}  // namespace ev
}  // namespace engine
