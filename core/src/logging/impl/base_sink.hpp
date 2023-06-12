#pragma once

#include <mutex>

#include <spdlog/sinks/sink.h>

#include <logging/impl/open_file_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class BaseSink : public spdlog::sinks::sink {
 public:
  ~BaseSink() override;
  BaseSink();

  void log(const spdlog::details::log_msg& msg) final;

  void flush() override;

  void set_pattern(const std::string& pattern) final;

  void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) final;

  virtual void Reopen(ReopenMode);

 protected:
  virtual void Write(std::string_view log) = 0;
  std::mutex& GetMutex();

 private:
  std::mutex mutex_;
  std::unique_ptr<spdlog::formatter> formatter_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
