#include <userver/tracing/tracer.hpp>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/uuid4.hpp>

#include <tracing/span_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

auto& GlobalNoLogSpans() {
    static rcu::Variable<NoLogSpans, rcu::ExclusiveRcuTraits> spans{};
    return spans;
}

auto& GlobalTracer() {
    static rcu::Variable<Tracer, rcu::ExclusiveRcuTraits> tracer("", nullptr);
    return tracer;
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

NoLogSpans CopyNoLogSpans() { return GlobalNoLogSpans().ReadCopy(); }

void Tracer::SetTracer(Tracer&& tracer) { GlobalTracer().Assign(std::move(tracer)); }

rcu::ReadablePtr<Tracer, rcu::ExclusiveRcuTraits> Tracer::ReadTracer() { return GlobalTracer().Read(); }

Tracer Tracer::CopyCurrentTracer() { return GlobalTracer().ReadCopy(); }

const std::string& Tracer::GetServiceName() const {
    UASSERT_MSG(
        !service_name_.empty(),
        "Requested a service name, which is misconfigured and empty. "
        "Set the `service-name` in the static config of the `tracer` "
        "component"
    );
    return service_name_;
}

TracerCleanupScope::TracerCleanupScope()
    : old_tracer_(Tracer::CopyCurrentTracer()), old_no_log_spans_(CopyNoLogSpans()) {}

TracerCleanupScope::~TracerCleanupScope() {
    SetNoLogSpans(std::move(old_no_log_spans_));
    Tracer::SetTracer(std::move(old_tracer_));
}

}  // namespace tracing

USERVER_NAMESPACE_END
