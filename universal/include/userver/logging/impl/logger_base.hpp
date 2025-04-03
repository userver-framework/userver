#pragma once

#include <atomic>

#include <boost/container/small_vector.hpp>

#include <userver/logging/format.hpp>
#include <userver/logging/impl/formatters/base.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;

}  // namespace logging

namespace logging::impl {

class TagWriter;
class LoggerBase;

/// Base logger class
class LoggerBase {
public:
    LoggerBase() = default;
    LoggerBase(const LoggerBase&) = delete;
    LoggerBase(LoggerBase&&) = delete;
    LoggerBase& operator=(const LoggerBase&) = delete;
    LoggerBase& operator=(LoggerBase&&) = delete;

    virtual ~LoggerBase();

    virtual void PrependCommonTags(impl::TagWriter writer) const;

    virtual void Log(Level, formatters::LoggerItemRef item) = 0;

    virtual formatters::BasePtr
    MakeFormatter(Level level, LogClass log_class, const utils::impl::SourceLocation& location) = 0;

    virtual void Flush() {}

    virtual void SetLevel(Level level);

    Level GetLevel() const noexcept;

    bool ShouldLog(Level level) const noexcept;

    void SetFlushOn(Level level);

    bool ShouldFlush(Level level) const;

    virtual void ForwardTo(LoggerBase* logger_to);

protected:
    virtual bool DoShouldLog(Level level) const noexcept;

private:
    std::atomic<Level> level_{Level::kNone};
    std::atomic<Level> flush_level_{Level::kWarning};
};

struct TextLogItem : formatters::LoggerItemBase {
    utils::SmallString<4096> log_line;

    TextLogItem() = default;
    explicit TextLogItem(std::string_view str) : log_line(str) {}
};

class TextLogger : public LoggerBase {
public:
    explicit TextLogger(Format format) : format_(format) {}

    Format GetFormat() const noexcept;

    formatters::BasePtr MakeFormatter(Level level, LogClass log_class, const utils::impl::SourceLocation& location)
        override;

private:
    const Format format_;
};

bool ShouldLogNoSpan(const LoggerBase& logger, Level level) noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
