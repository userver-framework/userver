#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

LoggerBase::LoggerBase(Format format) noexcept : format_(format) {}

LoggerBase::~LoggerBase() = default;

Format LoggerBase::GetFormat() const noexcept { return format_; }

void LoggerBase::SetLevel(Level level) { level_ = level; }

Level LoggerBase::GetLevel() const noexcept { return level_; }

bool LoggerBase::ShouldLog(Level level) const noexcept {
  return level_ <= level && level != Level::kNone;
}

void LoggerBase::SetFlushOn(Level level) { flush_level_ = level; }

bool LoggerBase::ShouldFlush(Level level) const {
  return flush_level_ <= level;
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
