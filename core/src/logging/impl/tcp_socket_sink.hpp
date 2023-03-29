#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>

#include <spdlog/common.h>
#include <spdlog/sinks/sink.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TcpSocketClient final {
 public:
  explicit TcpSocketClient(std::vector<engine::io::Sockaddr> addrs);

  TcpSocketClient(const TcpSocketClient&) = delete;
  TcpSocketClient(TcpSocketClient&&) = default;
  TcpSocketClient& operator=(const TcpSocketClient&) = delete;
  TcpSocketClient& operator=(TcpSocketClient&&) = default;
  ~TcpSocketClient();

  void Connect();
  void Send(const char* data, size_t n_bytes);
  bool IsConnected();
  void Close();

 private:
  std::vector<engine::io::Sockaddr> addrs_;
  engine::io::Socket socket_;
};

class TcpSocketSink final : public spdlog::sinks::sink {
 public:
  using spdlog::sinks::sink::sink;

  explicit TcpSocketSink(std::vector<engine::io::Sockaddr> addr);

  TcpSocketSink() = delete;

  void Close();

  ~TcpSocketSink() override = default;

  void log(const spdlog::details::log_msg& msg) final;

  void flush() final;

  void set_pattern(const std::string& pattern) final;

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) final;

 private:
  impl::TcpSocketClient client_;
  engine::Mutex client_mutex_;
  std::unique_ptr<spdlog::formatter> formatter_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
