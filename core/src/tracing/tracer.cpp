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

}  // namespace tracing

USERVER_NAMESPACE_END
