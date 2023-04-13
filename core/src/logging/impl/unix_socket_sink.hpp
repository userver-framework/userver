#pragma once

#include <mutex>
#include <string>

#include <spdlog/pattern_formatter.h>

#include <logging/impl/base_sink.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class UnixSocketClient final {
 public:
  UnixSocketClient() = default;

  UnixSocketClient(const UnixSocketClient&) = delete;
  UnixSocketClient(UnixSocketClient&&) = default;
  UnixSocketClient& operator=(const UnixSocketClient&) = delete;
  UnixSocketClient& operator=(UnixSocketClient&&) = default;
  ~UnixSocketClient();

  void connect(std::string_view filename);
  void send(std::string_view message);
  void close();

 private:
  int socket_{-1};
};

class UnixSocketSink final : public BaseSink {
 public:
  explicit UnixSocketSink(std::string_view filename) : filename_{filename} {
    client_.connect(filename_);
  }

  void Close();

 protected:
  void Write(std::string_view log) final;

 private:
  const std::string filename_;
  impl::UnixSocketClient client_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
