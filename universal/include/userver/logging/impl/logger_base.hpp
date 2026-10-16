#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string_view>

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
    LoggerBase() noexcept;
    LoggerBase(const LoggerBase&) = delete;
    LoggerBase(LoggerBase&&) = delete;
    LoggerBase& operator=(const LoggerBase&) = delete;
    LoggerBase& operator=(LoggerBase&&) = delete;

    virtual ~LoggerBase();

    virtual void PrependCommonTags(impl::TagWriter writer) const;

    virtual void Log(Level, formatters::LoggerItemRef item) = 0;

    virtual formatters::BasePtr MakeFormatter(
        Level level,
        LogClass log_class,
        const utils::impl::SourceLocation& location
    ) = 0;

    virtual void Flush() {}

    virtual void SetLevel(Level level);

    Level GetLevel() const noexcept;

    /// Sets the fraction of this logger's records below @ref logging::Level::kWarning to retain.
    /// Values are clamped to [0, 1] and rounded down to a multiple of 2^-32;
    /// `std::nullopt` disables sampling. A provided probability must be finite.
    ///
    /// Warning and higher records are never dropped by sampling. Span records use
    /// @ref tracing::SetTraceSamplingProbability instead. Locations force-enabled via
    /// dynamic debug log bypass this sampling.
    ///
    /// @note Migration from global sampling: replace `SetLogSamplingProbability` and
    /// `GetLogSamplingProbability` with this method and
    /// @ref logging::impl::LoggerBase::GetSamplingProbability on @ref logging::GetDefaultLogger.
    /// Each logger now has independent sampling state; replacing the default logger switches
    /// to the replacement's state. To configure named loggers, use the `logging` section of
    /// @ref USERVER_TELEMETRY_SETTINGS; `__default__` supplies the fallback for unspecified loggers.
    void SetSamplingProbability(std::optional<double> probability) noexcept;

    /// Returns this logger's quantized sampling probability, or `std::nullopt` when sampling is disabled.
    /// @see @ref logging::impl::LoggerBase::SetSamplingProbability
    std::optional<double> GetSamplingProbability() const noexcept;

    bool ShouldDropBySampling(Level level) const noexcept;

    bool ShouldLog(Level level) const noexcept;

    void SetFlushOn(Level level);

    bool ShouldFlush(Level level) const;

    virtual void ForwardTo(LoggerBase* logger_to);

private:
    std::atomic<Level> level_{Level::kNone};
    std::atomic<std::uint64_t> sampling_threshold_;
    std::atomic<Level> flush_level_{Level::kWarning};
};

struct TextLogItem : formatters::LoggerItemBase {
    utils::SmallString<4096> log_line;

    TextLogItem() = default;
    explicit TextLogItem(std::string_view str)
        : log_line(str)
    {}
};

class TextLogger : public LoggerBase {
public:
    explicit TextLogger(Format format)
        : format_(format)
    {}

    Format GetFormat() const noexcept;

    formatters::BasePtr MakeFormatter(Level level, LogClass log_class, const utils::impl::SourceLocation& location)
        override;

private:
    const Format format_;
};

inline bool ShouldLogNoSpan(const LoggerBase& logger, Level level) noexcept {
    return logger.GetLevel() <= level && level != Level::kNone;
}

bool ShouldLogWithSpanCheck(const LoggerBase& logger, Level level) noexcept;

}  // namespace logging::impl

USERVER_NAMESPACE_END
