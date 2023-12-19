#include <userver/tracing/tracer.hpp>

#include <atomic>

#include <userver/logging/impl/tag_writer.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/uuid4.hpp>

#include <tracing/no_log_spans.hpp>
#include <tracing/span_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

constexpr std::string_view kTraceIdName = "trace_id";
constexpr std::string_view kSpanIdName = "span_id";
constexpr std::string_view kParentIdName = "parent_id";

class NoopTracer final : public Tracer {
 public:
  NoopTracer(std::string_view service_name, logging::LoggerPtr logger)
      : Tracer(service_name, std::move(logger)) {}

  void LogSpanContextTo(const Span::Impl&,
                        logging::impl::TagWriter) const override;
};

void NoopTracer::LogSpanContextTo(const Span::Impl& span,
                                  logging::impl::TagWriter writer) const {
  writer.PutTag(kTraceIdName, span.GetTraceId());
  writer.PutTag(kSpanIdName, span.GetSpanId());
  writer.PutTag(kParentIdName, span.GetParentId());
}

auto& GlobalNoLogSpans() {
  static rcu::Variable<NoLogSpans> spans{};
  return spans;
}

auto& GlobalTracer() {
  static rcu::Variable<TracerPtr> tracer(tracing::MakeTracer({}, {}));
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
  GlobalTracer().Assign(std::move(tracer));
}

std::shared_ptr<Tracer> Tracer::GetTracer() {
  return GlobalTracer().ReadCopy();
}

const std::string& Tracer::GetServiceName() const {
  UASSERT_MSG(!service_name_.empty(),
              "Requested a service name, which is misconfigured and empty. "
              "Set the `service-name` in the static config of the `tracer` "
              "component");
  return service_name_;
}

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

tracing::TracerPtr MakeTracer(std::string_view service_name,
                              logging::LoggerPtr logger,
                              std::string_view tracer_type) {
  UINVARIANT(tracer_type == "native",
             "Only 'native' tracer type is available at the moment");
  return std::make_shared<tracing::NoopTracer>(service_name, std::move(logger));
}

}  // namespace tracing

USERVER_NAMESPACE_END
