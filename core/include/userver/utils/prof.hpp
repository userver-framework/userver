#pragma once

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>

// TODO: remove this file in TAXICOMMON-4666

USERVER_NAMESPACE_BEGIN

class LoggingTimeStorage : public tracing::impl::TimeStorage {
 public:
  explicit LoggingTimeStorage(std::string name)
      : TimeStorage(std::move(name)), log_extra_(nullptr) {}

  LoggingTimeStorage(std::string name, const logging::LogExtra& log_extra)
      : TimeStorage(std::move(name)), log_extra_(&log_extra) {}

  LoggingTimeStorage(std::string name, logging::LogExtra&& log_extra) = delete;

  LoggingTimeStorage(const LoggingTimeStorage&) = delete;
  LoggingTimeStorage& operator=(const LoggingTimeStorage&) = delete;

  ~LoggingTimeStorage() {
    if (log_extra_) {
      LOG_INFO() << GetLogs() << log_extra_;
    } else {
      LOG_INFO() << GetLogs();
    }
  }

 private:
  const logging::LogExtra* log_extra_;
};

using tracing::ScopeTime;
using tracing::impl::PerfTimePoint;
using tracing::impl::TimeStorage;

USERVER_NAMESPACE_END
