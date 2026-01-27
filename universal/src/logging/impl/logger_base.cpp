#include <userver/logging/impl/logger_base.hpp>

#include <logging/impl/formatters/json.hpp>
#include <logging/impl/formatters/tskv.hpp>
#include <userver/logging/impl/tag_writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

LoggerBase::~LoggerBase() = default;

void LoggerBase::PrependCommonTags(TagWriter) const {}

void LoggerBase::SetLevel(Level level) { level_ = level; }

Level LoggerBase::GetLevel() const noexcept { return level_; }

bool LoggerBase::ShouldLog(Level level) const noexcept { return ShouldLogNoSpan(*this, level) && DoShouldLog(level); }

void LoggerBase::SetFlushOn(Level level) { flush_level_ = level; }

bool LoggerBase::ShouldFlush(Level level) const { return flush_level_ <= level; }

void LoggerBase::ForwardTo(LoggerBase*) {}

bool LoggerBase::DoShouldLog(Level /*level*/) const noexcept { return true; }

formatters::BasePtr TextLogger::MakeFormatter(Level level, LogClass, const utils::impl::SourceLocation& location) {
    auto format = GetFormat();
    switch (format) {
        case Format::kLtsv:
        case Format::kTskv:
        case Format::kRaw:
            return std::make_unique<formatters::Tskv>(level, format, location);

        case Format::kJson:
        case Format::kJsonYaDeploy:
            return std::make_unique<formatters::Json>(level, format, location);
    }

    throw std::runtime_error("Invalid logger type");
}

Format TextLogger::GetFormat() const noexcept { return format_; }

bool ShouldLogNoSpan(const LoggerBase& logger, Level level) noexcept {
    return logger.GetLevel() <= level && level != Level::kNone;
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
