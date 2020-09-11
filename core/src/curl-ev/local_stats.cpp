#include <curl-ev/local_stats.hpp>

#include <cassert>

namespace curl {

LocalStats::LocalStats()
    : created_ts_(std::chrono::steady_clock::now()),
      open_socket_count_(0),
      retries_count_(0) {}

void LocalStats::mark_start_performing() { set(start_performing_ts_); }

void LocalStats::mark_start_read() { set(start_processing_ts_); }

void LocalStats::mark_start_write() { set(start_processing_ts_); }

void LocalStats::mark_complete() { set(complete_ts_); }

void LocalStats::mark_open_socket() { open_socket_count_++; }

void LocalStats::mark_retry() { retries_count_++; }

void LocalStats::set(LocalStats::time_point& ts) {
  static const time_point empty_;
  if (ts == empty_) {
    ts = std::chrono::steady_clock::now();
  }
}

LocalStats::duration LocalStats::time_to_start() const {
  return start_performing_ts_ - created_ts_;
}

LocalStats::duration LocalStats::time_to_connect() const {
  return start_processing_ts_ - start_performing_ts_;
}

LocalStats::duration LocalStats::time_to_process() const {
  return complete_ts_ - start_processing_ts_;
}

size_t LocalStats::open_socket_count() const { return open_socket_count_; }

size_t LocalStats::retries_count() const { return retries_count_; }

void LocalStats::reset() {
  start_performing_ts_ = {};
  start_processing_ts_ = {};
  complete_ts_ = {};
  open_socket_count_ = 0;
  retries_count_ = 0;
}

}  // namespace curl
