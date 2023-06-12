#pragma once

#include <mutex>
#include <string>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <spdlog/details/file_helper.h>
#include <spdlog/details/fmt_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/version.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

template <typename Mutex>
class ReopeningFileSink final : public spdlog::sinks::base_sink<Mutex> {
 public:
  using filename_t = spdlog::filename_t;
  using sink = spdlog::sinks::base_sink<Mutex>;

  ReopeningFileSink(filename_t filename) : filename_{std::move(filename)} {
    file_helper_.open(filename_);

    if (file_helper_.size() > 0) {
      spdlog::memory_buf_t formatted;
      spdlog::details::fmt_helper::append_string_view("\n", formatted);

      file_helper_.write(formatted);
    }
  }
  void Reopen(bool truncate) {
    std::lock_guard<Mutex> lock(this->mutex_);
    file_helper_.reopen(truncate);
  }
  void Close() { file_helper_.close(); }

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    sink::formatter_->format(msg, formatted);
    file_helper_.write(formatted);
  }

  void flush_() override { file_helper_.flush(); }

 private:
  filename_t filename_;
  spdlog::details::file_helper file_helper_;
};

using ReopeningFileSinkST = ReopeningFileSink<spdlog::details::null_mutex>;
using ReopeningFileSinkMT = ReopeningFileSink<std::mutex>;

}  // namespace logging::impl

USERVER_NAMESPACE_END
