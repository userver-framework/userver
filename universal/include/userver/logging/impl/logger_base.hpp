#pragma once

#include <atomic>

#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TagWriter;

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

  virtual void Log(Level level, std::string_view msg) = 0;

  virtual void Flush();

  virtual void PrependCommonTags(TagWriter writer) const;

  Format GetFormat() const noexcept;

  virtual void SetLevel(Level level);
  Level GetLevel() const noexcept;
  bool ShouldLog(Level level) const noexcept;

  void SetFlushOn(Level level);
  bool ShouldFlush(Level level) const;

 protected:
  virtual bool DoShouldLog(Level level) const noexcept;

 private:
  const Format format_;
  std::atomic<Level> level_{Level::kNone};
  std::atomic<Level> flush_level_{Level::kWarning};
};

bool ShouldLogNoSpan(const LoggerBase& logger, Level level) noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
