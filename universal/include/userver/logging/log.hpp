#pragma once

// Stub for logging support

USERVER_NAMESPACE_BEGIN

namespace logging {

class LogHelper {};

template <typename T>
inline LogHelper& operator<<(LogHelper& lh, T&&) {
  return lh;
}

template <typename T>
inline LogHelper operator<<(LogHelper&&, T&&) {
  return {};
}

inline void LogFlush() {}

}  // namespace logging

#ifdef LOG
#error "Attempt to stub out logging implementation"
#endif

#define LOG(lvl) \
  USERVER_NAMESPACE::logging::LogHelper {}

#define LOG_TRACE() \
  USERVER_NAMESPACE::logging::LogHelper {}
#define LOG_DEBUG() \
  USERVER_NAMESPACE::logging::LogHelper {}
#define LOG_INFO() \
  USERVER_NAMESPACE::logging::LogHelper {}
#define LOG_WARNING() \
  USERVER_NAMESPACE::logging::LogHelper {}
#define LOG_ERROR() \
  USERVER_NAMESPACE::logging::LogHelper {}
#define LOG_CRITICAL() \
  USERVER_NAMESPACE::logging::LogHelper {}

USERVER_NAMESPACE_END
