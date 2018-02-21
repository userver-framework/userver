#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace logging {

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

enum class Level {
  kTrace = spdlog::level::trace,
  kDebug = spdlog::level::debug,
  kInfo = spdlog::level::info,
  kWarning = spdlog::level::warn,
  kError = spdlog::level::err,
  kCritical = spdlog::level::critical
};

LoggerPtr& Log();

LoggerPtr DefaultLogger();

class LogHelper {
 public:
  LogHelper(Level level)
      : log_msg_(&Log()->name(),
                 static_cast<spdlog::level::level_enum>(level)) {}

  ~LogHelper() noexcept(false) { Log()->_sink_it(log_msg_); }

  template <typename T>
  friend LogHelper&& operator<<(LogHelper&& lh, const T& value) {
    lh << value;
    return std::move(lh);
  }

  template <typename T>
  friend LogHelper& operator<<(LogHelper& lh, const T& value) {
    lh.log_msg_.raw << value;
    return lh;
  }

 private:
  spdlog::details::log_msg log_msg_;
};

}  // namespace logging

#define LOG(lvl)                                                          \
  for (bool _need_log = ::logging::Log()->should_log(                     \
           static_cast<spdlog::level::level_enum>(lvl));                  \
       _need_log; _need_log = false)                                      \
  ::logging::LogHelper(lvl) << "module=" << __func__ << " ( " << __FILE__ \
                            << "," << __LINE__ << " ) \ttext="

#define LOG_TRACE() LOG(::logging::Level::kTrace)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
#define LOG_INFO() LOG(::logging::Level::kInfo)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
#define LOG_ERROR() LOG(::logging::Level::kError)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)
