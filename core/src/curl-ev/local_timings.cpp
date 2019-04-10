#include <curl-ev/local_timings.hpp>

#include <cassert>

namespace curl {

LocalTimings::LocalTimings() : created_ts_(std::chrono::steady_clock::now()) {}

void LocalTimings::mark_start_performing() { set(start_performing_ts_); }

void LocalTimings::mark_start_read() { set(start_processing_ts_); }

void LocalTimings::mark_start_write() { set(start_processing_ts_); }

void LocalTimings::mark_complete() { set(complete_ts_); }

void LocalTimings::set(LocalTimings::time_point& ts) {
  static const time_point empty_;
  if (ts == empty_) {
    ts = std::chrono::steady_clock::now();
  }
}

double LocalTimings::time_to_connect() const {
  return double_seconds(start_processing_ts_ - start_performing_ts_);
}

double LocalTimings::time_to_process() const {
  return double_seconds(complete_ts_ - start_processing_ts_);
}

double LocalTimings::double_seconds(const time_point::duration& duration) {
  using double_seconds_duration = std::chrono::duration<double, std::ratio<1>>;
  if (duration.count() >= 0) {
    return std::chrono::duration_cast<double_seconds_duration>(duration)
        .count();
  } else {
    return std::numeric_limits<double>::quiet_NaN();
  }
}

double LocalTimings::time_to_start() const {
  return double_seconds(start_performing_ts_ - created_ts_);
}

}  // namespace curl
