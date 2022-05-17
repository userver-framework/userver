#include <userver/logging/log.hpp>

#include <utility>

#include <logging/get_should_log_cache.hpp>
#include <logging/logger_with_info.hpp>
#include <logging/rate_limit.hpp>
#include <logging/spdlog.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/rcu/rcu.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
namespace {

auto& DefaultLoggerInternal() {
  static rcu::Variable<LoggerPtr> default_logger_ptr(
      MakeStderrLogger("default", Format::kTskv));
  return default_logger_ptr;
}

void UpdateLogLevelCache() {
  const auto& logger = DefaultLogger();
  for (int i = 0; i < kLevelMax + 1; i++)
    GetShouldLogCache()[i] = LoggerShouldLog(logger, static_cast<Level>(i));

  GetShouldLogCache()[static_cast<int>(Level::kNone)] = false;
}

constexpr bool IsPowerOf2(uint64_t n) { return (n & (n - 1)) == 0; }

}  // namespace

LoggerPtr DefaultLogger() { return DefaultLoggerInternal().ReadCopy(); }

LoggerPtr DefaultLoggerOptional() noexcept {
  try {
    return DefaultLogger();
  } catch (...) {
    UASSERT(false);
  }

  return {};
}

LoggerPtr SetDefaultLogger(LoggerPtr logger) {
  UASSERT(logger);
  if (engine::current_task::GetCurrentTaskContextUnchecked() == nullptr) {
    // TODO TAXICOMMON-4233 remove
    engine::RunStandalone([&logger] { logger = SetDefaultLogger(logger); });
    return logger;
  }

  auto ptr = DefaultLoggerInternal().StartWrite();
  swap(*ptr, logger);
  ptr.Commit();

  UpdateLogLevelCache();
  return logger;
}

void SetDefaultLoggerLevel(Level level) {
  DefaultLogger()->ptr->set_level(
      static_cast<spdlog::level::level_enum>(level));
  UpdateLogLevelCache();
}

void SetLoggerLevel(LoggerPtr logger, Level level) {
  logger->ptr->set_level(static_cast<spdlog::level::level_enum>(level));
}

Level GetDefaultLoggerLevel() {
  return static_cast<Level>(DefaultLogger()->ptr->level());
}

bool LoggerShouldLog(const LoggerPtr& logger, Level level) {
  return logger &&
         logger->ptr->should_log(static_cast<spdlog::level::level_enum>(level));
}

Level GetLoggerLevel(const LoggerPtr& logger) {
  return static_cast<Level>(logger->ptr->level());
}

void LogFlush() { DefaultLogger()->ptr->flush(); }

void LogFlush(LoggerPtr logger) { logger->ptr->flush(); }

namespace impl {

RateLimiter::RateLimiter(LoggerPtr logger, RateLimitData& data,
                         Level level) noexcept
    : level_(level), should_log_(false), dropped_count_(0) {
  try {
    should_log_ = logging::LoggerShouldLog(logger, level);
    if (!should_log_) return;

    if (!impl::IsLogLimitedEnabled()) {
      return;
    }

    const auto reset_interval = impl::GetLogLimitedInterval();
    const auto now = std::chrono::steady_clock::now();

    if (now - data.last_reset_time >= reset_interval) {
      data.count_since_reset = 0;
      data.last_reset_time = now;
    }

    if (IsPowerOf2(++data.count_since_reset)) {
      // log the current message together with the dropped count
      dropped_count_ = std::exchange(data.dropped_count, 0);
    } else {
      // drop the current message
      ++data.dropped_count;
      should_log_ = false;
    }
  } catch (const std::exception& e) {
    UASSERT_MSG(false, e.what());
    should_log_ = false;
  }
}

LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl) noexcept {
  if (rl.dropped_count_ != 0) {
    lh << "[" << rl.dropped_count_ << " logs dropped] ";
  }
  return lh;
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
