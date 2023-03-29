#pragma once

#include <mutex>
#include <string_view>

#include <spdlog/sinks/sink.h>

#include <userver/fs/blocking/file_descriptor.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class FdSink : public spdlog::sinks::sink {
 public:
  FdSink() = delete;
  explicit FdSink(fs::blocking::FileDescriptor fd);

  ~FdSink() override;

  void log(const spdlog::details::log_msg& msg) final;

  void flush() override;

  void set_pattern(const std::string& pattern) final;

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) final;

 protected:
  std::mutex& GetMutex();
  fs::blocking::FileDescriptor& GetFd();
  void SetFd(fs::blocking::FileDescriptor&& fd);

 private:
  fs::blocking::FileDescriptor fd_;
  std::mutex mutex_;
  std::unique_ptr<spdlog::formatter> formatter_;
};

class StdoutSink final : public FdSink {
 public:
  StdoutSink();
  ~StdoutSink() final;
  void flush() final;
};

class StderrSink final : public FdSink {
 public:
  StderrSink();
  ~StderrSink() final;
  void flush() final;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
