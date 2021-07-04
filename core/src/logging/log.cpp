#include <userver/logging/log.hpp>

#include <utility>

#include <engine/task/task_context.hpp>
#include <logging/rate_limit.hpp>
#include <logging/spdlog.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/rcu/rcu.hpp>

namespace logging {
namespace {

auto& DefaultLoggerInternal() {
  static rcu::Variable<LoggerPtr> default_logger_ptr(
      MakeStderrLogger("default"));
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

LoggerPtr SetDefaultLogger(LoggerPtr logger) {
  if (engine::current_task::GetCurrentTaskContextUnchecked() == nullptr) {
    RunInCoro([&logger] { logger = SetDefaultLogger(logger); }, 1);
    return logger;
  }

  auto ptr = DefaultLoggerInternal().StartWrite();
  swap(*ptr, logger);
  ptr.Commit();

  UpdateLogLevelCache();
  return logger;
}

void SetDefaultLoggerLevel(Level level) {
  DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
  UpdateLogLevelCache();
}

void SetLoggerLevel(LoggerPtr logger, Level level) {
  logger->set_level(static_cast<spdlog::level::level_enum>(level));
}

Level GetDefaultLoggerLevel() {
  return static_cast<Level>(DefaultLogger()->level());
}

bool LoggerShouldLog(const LoggerPtr& logger, Level level) {
  return logger->should_log(static_cast<spdlog::level::level_enum>(level));
}

Level GetLoggerLevel(const LoggerPtr& logger) {
  return static_cast<Level>(logger->level());
}

void LogFlush() { DefaultLogger()->flush(); }

void LogFlush(LoggerPtr logger) { logger->flush(); }

namespace impl {

RateLimiter::RateLimiter(LoggerPtr logger, RateLimitData& data, Level level)
    : level_(level),
      should_log_(logging::LoggerShouldLog(logger, level)),
      dropped_count_(0) {
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
}

LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl) {
  if (rl.dropped_count_ != 0) {
    lh << "[" << rl.dropped_count_ << " logs dropped] ";
  }
  return lh;
}

}  // namespace impl

}  // namespace logging
