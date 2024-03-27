#include <userver/logging/log.hpp>

#include <utility>

#include <logging/dynamic_debug.hpp>
#include <logging/rate_limit.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/assert.hpp>

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

void SetDefaultLoggerRef(LoggerRef logger) noexcept {
  NonOwningDefaultLoggerInternal() = &logger;
}

bool has_background_threads_which_can_log{false};

}  // namespace impl

LoggerRef GetDefaultLogger() noexcept {
  return *NonOwningDefaultLoggerInternal().load();
}

DefaultLoggerGuard::DefaultLoggerGuard(LoggerPtr new_default_logger) noexcept
    : logger_prev_(GetDefaultLogger()),
      level_prev_(GetDefaultLoggerLevel()),
      logger_new_(std::move(new_default_logger)) {
  UASSERT(logger_new_);
  logging::impl::SetDefaultLoggerRef(*logger_new_);
}

DefaultLoggerGuard::~DefaultLoggerGuard() {
  UASSERT_MSG(
      !impl::has_background_threads_which_can_log,
      "DefaultLoggerGuard with a new logger should outlive the coroutine "
      "engine, because otherwise it could be in use right now, when the "
      "~DefaultLoggerGuard() is called and the logger is destroyed. "
      "Construct the DefaultLoggerGuard before calling engine::RunStandalone; "
      "in tests use the utest::DefaultLoggerFixture.");

  logging::impl::SetDefaultLoggerRef(logger_prev_);
  logging::SetDefaultLoggerLevel(level_prev_);
}

DefaultLoggerLevelScope::DefaultLoggerLevelScope(logging::Level level)
    : logger_(GetDefaultLogger()), level_initial_(GetLoggerLevel(logger_)) {
  SetLoggerLevel(logger_, level);
}

DefaultLoggerLevelScope::~DefaultLoggerLevelScope() {
  SetLoggerLevel(logger_, level_initial_);
}

void SetDefaultLoggerLevel(Level level) { GetDefaultLogger().SetLevel(level); }

void SetLoggerLevel(LoggerRef logger, Level level) { logger.SetLevel(level); }

Level GetDefaultLoggerLevel() noexcept {
  static_assert(noexcept(GetDefaultLogger().GetLevel()));
  return GetDefaultLogger().GetLevel();
}

bool ShouldLog(Level level) noexcept {
  static_assert(noexcept(GetDefaultLogger().ShouldLog(level)));
  return GetDefaultLogger().ShouldLog(level);
}

bool LoggerShouldLog(LoggerRef logger, Level level) noexcept {
  static_assert(noexcept(logger.ShouldLog(level)));
  return logger.ShouldLog(level);
}

bool LoggerShouldLog(const LoggerPtr& logger, Level level) noexcept {
  return logger && LoggerShouldLog(*logger, level);
}

Level GetLoggerLevel(LoggerRef logger) noexcept {
  static_assert(noexcept(logger.GetLevel()));
  return logger.GetLevel();
}

void LogFlush() { GetDefaultLogger().Flush(); }

void LogFlush(LoggerRef logger) { logger.Flush(); }

namespace impl {

RateLimiter::RateLimiter(RateLimitData& data, Level level) noexcept
    : level_(level) {
  try {
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

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
StaticLogEntry::StaticLogEntry(const char* path, int line) noexcept {
  static_assert(sizeof(LogEntryContent) == sizeof(content_));
  // static_assert(std::is_trivially_destructible_v<LogEntryContent>);
  auto* item = new (&content_) LogEntryContent(path, line);
  RegisterLogLocation(*item);
}

bool StaticLogEntry::ShouldNotLog(logging::LoggerRef logger,
                                  logging::Level level) const noexcept {
  const auto& content = reinterpret_cast<const LogEntryContent&>(content_);
  const auto state = content.state.load();
  const bool force_disabled = level < state.force_disabled_level_plus_one;
  const bool force_enabled =
      level >= state.force_enabled_level && level != logging::Level::kNone;
  return (!LoggerShouldLog(logger, level) || force_disabled) && !force_enabled;
}

bool StaticLogEntry::ShouldNotLog(const logging::LoggerPtr& logger,
                                  logging::Level level) const noexcept {
  return !logger || ShouldNotLog(*logger, level);
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
