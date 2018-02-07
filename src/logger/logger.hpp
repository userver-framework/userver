#pragma once

#include <spdlog/spdlog.h>

namespace logger {

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

}  // namespace logger

#define LOG(lvl)                                                              \
  for (bool _need_log = logger::Log()->should_log(                            \
           static_cast<spdlog::level::level_enum>(lvl));                      \
       _need_log; _need_log = false)                                          \
  logger::LogHelper(lvl) << "module=" << __func__ << " ( " << __FILE__ << "," \
                         << __LINE__ << " ) \ttext="

#define LOG_TRACE() LOG(logger::Level::kTrace)
#define LOG_DEBUG() LOG(logger::Level::kDebug)
#define LOG_INFO() LOG(logger::Level::kInfo)
#define LOG_WARNING() LOG(logger::Level::kWarning)
#define LOG_ERROR() LOG(logger::Level::kError)
#define LOG_CRITICAL() LOG(logger::Level::kCritical)
