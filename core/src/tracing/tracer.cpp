#include <userver/tracing/tracer.hpp>

#include <atomic>

#include <tracing/no_log_spans.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/tracing/noop.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

auto& GlobalNoLogSpans() {
  static rcu::Variable<NoLogSpans> spans{};
  return spans;
}

auto& GlobalTracer() {
  static const std::string kEmptyServiceName;
  static rcu::Variable<TracerPtr> tracer(
      tracing::MakeNoopTracer(kEmptyServiceName));
  return tracer;
}

template <class T>
bool ValueMatchPrefix(const T& value, const T& prefix) {
  return prefix.size() <= value.size() &&
         value.compare(0, prefix.size(), prefix) == 0;
}

template <class T>
bool ValueMatchesOneOfPrefixes(const T& value,
                               const boost::container::flat_set<T>& prefixes) {
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

Tracer::~Tracer() = default;

void Tracer::SetNoLogSpans(NoLogSpans&& spans) {
  auto& global_spans = GlobalNoLogSpans();
  global_spans.Assign(std::move(spans));
}

bool Tracer::IsNoLogSpan(const std::string& name) {
  const auto spans = GlobalNoLogSpans().Read();

  return ValueMatchesOneOfPrefixes(name, spans->prefixes) ||
         spans->names.find(name) != spans->names.end();
}

void Tracer::SetTracer(std::shared_ptr<Tracer> tracer) {
  GlobalTracer().Assign(tracer);
}

std::shared_ptr<Tracer> Tracer::GetTracer() {
  return GlobalTracer().ReadCopy();
}

const std::string& Tracer::GetServiceName() const { return service_name_; }

Span Tracer::CreateSpanWithoutParent(std::string name) {
  auto span =
      Span(shared_from_this(), std::move(name), nullptr, ReferenceType::kChild);

  span.SetLink(utils::generators::GenerateUuid());
  return span;
}

Span Tracer::CreateSpan(std::string name, const Span& parent,
                        ReferenceType reference_type) {
  return Span(shared_from_this(), std::move(name), &parent, reference_type);
}

}  // namespace tracing

USERVER_NAMESPACE_END
