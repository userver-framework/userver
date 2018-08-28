#pragma once

#include <chrono>

namespace curl {
class LocalTimings {
 public:
  LocalTimings();
  void mark_start_performing();
  void mark_start_read();
  void mark_start_write();
  void mark_complete();

  double time_to_start() const;
  double time_to_connect() const;
  double time_to_process() const;

 private:
  using time_point = std::chrono::steady_clock::time_point;

  time_point created_ts_;
  time_point start_performing_ts_;
  time_point start_processing_ts_;
  time_point complete_ts_;

  static void set(time_point& ts);
  static double double_seconds(const time_point::duration& duration);
};
}  // namespace curl
