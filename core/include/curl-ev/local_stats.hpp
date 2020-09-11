#pragma once

#include <chrono>

namespace curl {

/// Represents all the local timings and statistics
class LocalStats final {
 public:
  using time_point = std::chrono::steady_clock::time_point;
  using duration = time_point::duration;

  LocalStats();
  void mark_start_performing();
  void mark_start_read();
  void mark_start_write();
  void mark_complete();
  void mark_open_socket();
  void mark_retry();

  duration time_to_start() const;
  duration time_to_connect() const;

  /// total time
  duration time_to_process() const;

  size_t open_socket_count() const;

  /// returns 0 based retires count. In other words:
  ///   0 - the very first request succeeded
  ///   1 - made 1 retry
  ///   2 - made 2 retries
  ///   ...
  ///
  size_t retries_count() const;

  void reset();

 private:
  const time_point created_ts_;
  time_point start_performing_ts_;
  time_point start_processing_ts_;
  time_point complete_ts_;
  size_t open_socket_count_;
  size_t retries_count_;

  static void set(time_point& ts);
};
}  // namespace curl
