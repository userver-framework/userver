#pragma once

#include <mutex>
#include <string>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/sink.h>

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

class UnixSocketSink final : public spdlog::sinks::sink {
 public:
  explicit UnixSocketSink(std::string_view filename)
      : filename_{filename},
        formatter_{std::make_unique<spdlog::pattern_formatter>()} {
    client_.connect(filename_);
  }

  void Close();

  void log(const spdlog::details::log_msg& msg) final;

  void flush() override {}

  void set_pattern(const std::string& pattern) final;

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) final;

 private:
  const std::string filename_;
  std::mutex mutex_;
  impl::UnixSocketClient client_;
  std::unique_ptr<spdlog::formatter> formatter_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
