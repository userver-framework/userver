#include <userver/logging/log.hpp>

#include <utility>

#include <engine/task/task_context.hpp>
#include <logging/rate_limit.hpp>
#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
namespace {

auto& NonOwningDefaultLoggerInternal() noexcept {
  static std::atomic<impl::LoggerBase*> default_logger_ptr{&GetNullLogger()};
  return default_logger_ptr;
}

constexpr bool IsPowerOf2(uint64_t n) { return (n & (n - 1)) == 0; }

}  // namespace

namespace impl {

LoggerRef DefaultLoggerRef() noexcept {
  return *NonOwningDefaultLoggerInternal().load();
}

void SetDefaultLoggerRef(LoggerRef logger) noexcept {
  NonOwningDefaultLoggerInternal() = &logger;
}

}  // namespace impl

DefaultLoggerGuard::DefaultLoggerGuard(LoggerPtr new_default_logger) noexcept
    : logger_prev_(impl::DefaultLoggerRef()),
      level_prev_(GetDefaultLoggerLevel()),
      logger_new_(std::move(new_default_logger)) {
  UASSERT(logger_new_);
  logging::impl::SetDefaultLoggerRef(*logger_new_);
}

DefaultLoggerGuard::~DefaultLoggerGuard() {
  logging::impl::SetDefaultLoggerRef(logger_prev_);
  logging::SetDefaultLoggerLevel(level_prev_);

  UASSERT_MSG(
      !engine::current_task::GetCurrentTaskContextUnchecked(),
      "DefaultLoggerGuard with a new logger should outlive the coroutine "
      "engine, because otherwise it could be in use right now, when the "
      "~DefaultLoggerGuard() is called and the logger is destroyed. "
      "Construct the DefaultLoggerGuard before calling engine::RunStandalone; "
      "in tests use the utest::DefaultLoggerFixture.");
}

void SetDefaultLoggerLevel(Level level) {
  impl::DefaultLoggerRef().SetLevel(level);
}

void SetLoggerLevel(LoggerRef logger, Level level) { logger.SetLevel(level); }

Level GetDefaultLoggerLevel() noexcept {
  static_assert(noexcept(impl::DefaultLoggerRef().GetLevel()));
  return impl::DefaultLoggerRef().GetLevel();
}

bool LoggerShouldLog(LoggerCRef logger, Level level) noexcept {
  static_assert(noexcept(logger.ShouldLog(level)));
  return logger.ShouldLog(level);
}

bool LoggerShouldLog(const LoggerPtr& logger, Level level) noexcept {
  return logger && LoggerShouldLog(*logger, level);
}

Level GetLoggerLevel(LoggerCRef logger) noexcept {
  static_assert(noexcept(logger.GetLevel()));
  return logger.GetLevel();
}

void LogFlush() { impl::DefaultLoggerRef().Flush(); }

void LogFlush(LoggerCRef logger) { logger.Flush(); }

namespace impl {

RateLimiter::RateLimiter(LoggerCRef logger, RateLimitData& data,
                         Level level) noexcept
    : level_(level) {
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
