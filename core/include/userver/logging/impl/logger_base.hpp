#pragma once

#include <atomic>

#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

/// Base logger class
class LoggerBase {
 public:
  LoggerBase() = delete;
  LoggerBase(const LoggerBase&) = delete;
  LoggerBase(LoggerBase&&) = delete;
  LoggerBase& operator=(const LoggerBase&) = delete;
  LoggerBase& operator=(LoggerBase&&) = delete;

  explicit LoggerBase(Format format) noexcept;

  virtual ~LoggerBase();

  virtual void Log(Level level, std::string_view msg) const = 0;

  virtual void Flush() const = 0;

  Format GetFormat() const noexcept;

  virtual void SetLevel(Level level);
  Level GetLevel() const noexcept;
  bool ShouldLog(Level level) const noexcept;

  void SetFlushOn(Level level);
  bool ShouldFlush(Level level) const;

 private:
  const Format format_;
  std::atomic<Level> level_{Level::kNone};
  std::atomic<Level> flush_level_{Level::kWarning};
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
