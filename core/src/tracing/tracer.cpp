#include <userver/tracing/tracer.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/uuid4.hpp>

#include <tracing/span_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

constexpr int kInheritDefaultLoggerLevel = -1;

constinit std::atomic<int> span_log_level{kInheritDefaultLoggerLevel};
static_assert(decltype(span_log_level)::is_always_lock_free);

constexpr double kTraceSamplingRange = 0x1p64;
constexpr std::uint64_t kTraceSamplingDisabled = std::numeric_limits<std::uint64_t>::max();
constexpr std::uint64_t kTraceSamplingAll = kTraceSamplingDisabled - 1;
static_assert(std::numeric_limits<double>::radix == 2);
static_assert(std::numeric_limits<double>::digits + 1 < std::numeric_limits<std::uint64_t>::digits);

constinit std::atomic<std::uint64_t> trace_sampling_threshold{kTraceSamplingDisabled};
static_assert(decltype(trace_sampling_threshold)::is_always_lock_free);

void ResetTracingState() {
    SetNoLogSpans({});
    SetSpanLogLevel(std::nullopt);
    SetTraceSamplingProbability(std::nullopt);
}

std::uint64_t TraceIdHash(std::string_view trace_id) noexcept {
    // Take the lowest 8 bytes of a hex trace id, as the opentelemetry
    // TraceIdRatioBased sampler does, so that the decision for a trace is the
    // same in every service it passes through.
    constexpr std::size_t kHexDigits = 16;
    if (trace_id.size() >= kHexDigits) {
        std::uint64_t value{};
        const auto* const last = trace_id.data() + trace_id.size();
        const auto result = std::from_chars(last - kHexDigits, last, value, 16);
        if (result.ec == std::errc{} && result.ptr == last) {
            return value;
        }
    }
    // FNV-1a for non-hex trace ids: unlike std::hash it is stable across
    // stdlib implementations, keeping the decision consistent between services
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char symbol : trace_id) {
        hash ^= static_cast<unsigned char>(symbol);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

auto& GlobalNoLogSpans() {
    static rcu::Variable<NoLogSpans, rcu::ExclusiveRcuTraits> spans{};
    return spans;
}

template <class T>
bool ValueMatchPrefix(const T& value, const T& prefix) {
    return prefix.size() <= value.size() && value.compare(0, prefix.size(), prefix) == 0;
}

template <class T>
bool ValueMatchesOneOfPrefixes(const T& value, const boost::container::flat_set<T>& prefixes) {
    for (const auto& prefix : prefixes) {
        if (ValueMatchPrefix(value, prefix)) {
            return true;
        }

        if (value < prefix) {
            break;
        }
    }

    return false;
}

}  // namespace

void SetNoLogSpans(NoLogSpans&& spans) {
    auto& global_spans = GlobalNoLogSpans();
    global_spans.Assign(std::move(spans));
}

bool IsNoLogSpan(const std::string& name) {
    const auto spans = GlobalNoLogSpans().Read();

    return ValueMatchesOneOfPrefixes(name, spans->prefixes) || spans->names.find(name) != spans->names.end();
}

void SetSpanLogLevel(std::optional<logging::Level> log_level) noexcept {
    span_log_level.store(log_level ? static_cast<int>(*log_level) : kInheritDefaultLoggerLevel);
}

std::optional<logging::Level> GetSpanLogLevel() noexcept {
    const auto log_level = span_log_level.load();
    if (log_level == kInheritDefaultLoggerLevel) {
        return std::nullopt;
    }
    return static_cast<logging::Level>(log_level);
}

void SetTraceSamplingProbability(std::optional<double> probability) noexcept {
    auto value = kTraceSamplingDisabled;
    if (probability.has_value() && !std::isnan(probability.value())) {
        const auto clamped_probability = std::clamp(probability.value(), 0.0, 1.0);
        // 2^64 does not fit in uint64_t, so full sampling has a separate sentinel.
        // For probabilities below 1, double precision leaves both sentinels
        // above all representable thresholds. Conversion rounds down to 2^-64.
        value =
            clamped_probability == 1.0
                ? kTraceSamplingAll
                : static_cast<std::uint64_t>(clamped_probability * kTraceSamplingRange);
    }
    trace_sampling_threshold.store(value, std::memory_order_relaxed);
}

std::optional<double> GetTraceSamplingProbability() noexcept {
    const auto threshold = trace_sampling_threshold.load(std::memory_order_relaxed);
    if (threshold == kTraceSamplingDisabled) {
        return std::nullopt;
    }
    if (threshold == kTraceSamplingAll) {
        return 1.0;
    }
    return static_cast<double>(threshold) / kTraceSamplingRange;
}

namespace impl {

bool ShouldSampleTrace(std::string_view trace_id) noexcept {
    const auto threshold = trace_sampling_threshold.load(std::memory_order_relaxed);
    if (threshold >= kTraceSamplingAll) {
        return true;
    }
    if (threshold == 0) {
        return false;
    }
    return TraceIdHash(trace_id) < threshold;
}

}  // namespace impl

TracingStateGuard::TracingStateGuard()
    : previous_no_log_spans_(GlobalNoLogSpans().ReadCopy()),
      previous_span_log_level_(GetSpanLogLevel()),
      previous_trace_sampling_probability_(GetTraceSamplingProbability())
{
    ResetTracingState();
}

TracingStateGuard::~TracingStateGuard() {
    SetNoLogSpans(std::move(previous_no_log_spans_));
    SetSpanLogLevel(previous_span_log_level_);
    SetTraceSamplingProbability(previous_trace_sampling_probability_);
}

}  // namespace tracing

USERVER_NAMESPACE_END
