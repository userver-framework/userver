#include <userver/tracing/tracer.hpp>

#include <atomic>
#include <optional>

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

void ResetTracingState() {
    SetNoLogSpans({});
    SetSpanLogLevel(std::nullopt);
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

TracingStateGuard::TracingStateGuard() { ResetTracingState(); }

TracingStateGuard::~TracingStateGuard() { ResetTracingState(); }

}  // namespace tracing

USERVER_NAMESPACE_END
