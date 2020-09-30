#pragma once

// Stub for logging support

namespace logging {
namespace impl {

struct LoggerStub {};

template <typename T>
inline LoggerStub operator<<(LoggerStub, T&&) {
  return {};
}

}  // namespace impl

inline void LogFlush() {}

}  // namespace logging

#ifdef LOG
#error "Attempt to stub out logging implementation"
#endif

#define LOG(lvl) \
  ::logging::impl::LoggerStub {}

#define LOG_TRACE() \
  ::logging::impl::LoggerStub {}
#define LOG_DEBUG() \
  ::logging::impl::LoggerStub {}
#define LOG_INFO() \
  ::logging::impl::LoggerStub {}
#define LOG_WARNING() \
  ::logging::impl::LoggerStub {}
#define LOG_ERROR() \
  ::logging::impl::LoggerStub {}
#define LOG_CRITICAL() \
  ::logging::impl::LoggerStub {}
