#pragma once

#include <mutex>
#include <string>

#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class UnixSocketClient final {
 public:
  UnixSocketClient() = default;

  UnixSocketClient(const UnixSocketClient&) = delete;
  UnixSocketClient(UnixSocketClient&&) = default;
  UnixSocketClient& operator=(const UnixSocketClient&) = delete;
  UnixSocketClient& operator=(UnixSocketClient&&) = default;
  ~UnixSocketClient();

  void connect(const spdlog::filename_t& filename);
  void send(const char* data, size_t n_bytes);
  void close();

 private:
  int socket_{-1};
};

}  // namespace impl

template <typename Mutex>
class SocketSink final : public spdlog::sinks::base_sink<Mutex> {
 public:
  using filename_t = spdlog::filename_t;
  using sink = spdlog::sinks::base_sink<Mutex>;

  explicit SocketSink(filename_t filename) : filename_{std::move(filename)} {
    client_.connect(filename_);
  }

  // TODO : find out is it necessary to close and where. Destructor or close?
  void Close() { client_.close(); }

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t formatted;
    sink::formatter_->format(msg, formatted);
    client_.send(formatted.data(), formatted.size());
  }

  void flush_() override {}

 private:
  filename_t filename_;
  impl::UnixSocketClient client_;
};

using SocketSinkST = SocketSink<spdlog::details::null_mutex>;
using SocketSinkMT = SocketSink<std::mutex>;

}  // namespace logging

USERVER_NAMESPACE_END
