#include <userver/logging/impl/logger_base.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>

#include <logging/impl/formatters/json.hpp>
#include <logging/impl/formatters/tskv.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
namespace {

constexpr std::uint64_t kSamplingRange = std::uint64_t{1} << 32;
constexpr std::uint64_t kSamplingDisabled = std::numeric_limits<std::uint64_t>::max();

}  // namespace

LoggerBase::LoggerBase() noexcept : sampling_threshold_(kSamplingDisabled) {}

LoggerBase::~LoggerBase() = default;

void LoggerBase::PrependCommonTags(TagWriter) const {}

void LoggerBase::SetLevel(Level level) { level_ = level; }

Level LoggerBase::GetLevel() const noexcept { return level_; }

void LoggerBase::SetSamplingProbability(std::optional<double> probability) noexcept {
    static_assert(decltype(sampling_threshold_)::is_always_lock_free);
    auto value = kSamplingDisabled;
    if (probability.has_value()) {
        UASSERT(std::isfinite(*probability));
        value = static_cast<std::uint64_t>(std::clamp(*probability, 0.0, 1.0) * kSamplingRange);
    }
    sampling_threshold_.store(value, std::memory_order_relaxed);
}

std::optional<double> LoggerBase::GetSamplingProbability() const noexcept {
    const auto threshold = sampling_threshold_.load(std::memory_order_relaxed);
    if (threshold == kSamplingDisabled) {
        return std::nullopt;
    }
    return static_cast<double>(threshold) / kSamplingRange;
}

bool LoggerBase::ShouldDropBySampling(Level level) const noexcept {
    if (level >= Level::kWarning) {
        return false;
    }
    const auto threshold = sampling_threshold_.load(std::memory_order_relaxed);
    if (threshold >= kSamplingRange) {
        return false;
    }
    if (threshold == 0) {
        return true;
    }
    try {
        return utils::Rand() >= threshold;
    } catch (const std::exception& e) {
        UASSERT_MSG(false, e.what());
        return false;
    }
}

bool LoggerBase::ShouldLog(Level level) const noexcept {
    return level != Level::kNone && ShouldLogWithSpanCheck(*this, level);
}

void LoggerBase::SetFlushOn(Level level) { flush_level_ = level; }

bool LoggerBase::ShouldFlush(Level level) const { return flush_level_ <= level; }

void LoggerBase::ForwardTo(LoggerBase*) {}

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

__attribute__((weak)) bool ShouldLogWithSpanCheck(const LoggerBase& logger, Level level) noexcept {
    return ShouldLogNoSpan(logger, level);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
