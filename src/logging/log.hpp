#pragma once

#include <memory>

#include <spdlog/common.h>

#include <utils/encoding/tskv.hpp>

#include "level.hpp"
#include "log_extra.hpp"
#include "logger.hpp"

namespace logging {

LoggerPtr& Log();

class LogHelper {
 public:
  LogHelper(Level level, const char* path, int line, const char* func);
  ~LogHelper() noexcept(false) { DoLog(); }

  template <typename T>
  friend LogHelper&& operator<<(LogHelper&& lh, const T& value) {
    lh << value;
    return std::move(lh);
  }

  template <typename T>
  friend LogHelper& operator<<(LogHelper& lh, const std::atomic<T>& value) {
    return lh << value.load();
  }

  template <typename T>
  friend
      typename std::enable_if<!utils::encoding::TypeNeedsEncodeTskv<T>::value,
                              LogHelper&>::type
      operator<<(LogHelper& lh, const T& value) {
    lh.log_msg_.raw << value;
    return lh;
  }

  template <typename T>
  friend typename std::enable_if<utils::encoding::TypeNeedsEncodeTskv<T>::value,
                                 LogHelper&>::type
  operator<<(LogHelper& lh, const T& value) {
    utils::encoding::EncodeTskv(lh.log_msg_.raw, value,
                                utils::encoding::EncodeTskvMode::kValue);
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, LogExtra extra) {
    lh.extra_.Extend(std::move(extra));
    return lh;
  }

 private:
  void DoLog();

  void AppendLogExtra();
  void LogTextKey();
  void LogModule(const char* path, int line, const char* func);

  spdlog::details::log_msg log_msg_;
  LogExtra extra_;
};

}  // namespace logging

#define LOG(lvl)                                                        \
  for (bool _need_log =                                                 \
           ::logging::Log()->should_log(::logging::ToSpdlogLevel(lvl)); \
       _need_log; _need_log = false)                                    \
  ::logging::LogHelper(lvl, FILENAME, __LINE__, __func__)

#define LOG_TRACE() LOG(::logging::Level::kTrace)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
#define LOG_INFO() LOG(::logging::Level::kInfo)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
#define LOG_ERROR() LOG(::logging::Level::kError)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)

#define LOG_FLUSH() ::logging::Log()->flush()
