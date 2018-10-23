#include <tracing/span.hpp>
#include <tracing/span_impl.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/format.hpp>

#include <tracing/tracer.hpp>
#include <utils/uuid4.hpp>

namespace tracing {

namespace {

const std::string kLinkTag = "link";
using RealMilliseconds = std::chrono::duration<double, std::milli>;

const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";

const std::string kReferenceType = "span_ref_type";
const std::string kReferenceTypeChild = "child";
const std::string kReferenceTypeFollows = "follows";

}  // namespace

Span::Impl::Impl(TracerPtr tracer, const std::string& name,
                 const Span::Impl* parent, ReferenceType reference_type)
    : tracer(std::move(tracer)),
      name_(name),
      start_system_time_(std::chrono::system_clock::now()),
      start_steady_time_(std::chrono::steady_clock::now()),
      trace_id_(parent ? parent->GetTraceId()
                       : utils::generators::GenerateUuid()),
      span_id_(utils::generators::GenerateUuid()),
      parent_id_(parent ? parent->GetParentId() : ""),
      reference_type_(reference_type) {}

Span::Impl::~Impl() {
  const double start_ts =
      start_system_time_.time_since_epoch().count() / 1000000000.0;

  std::array<char, 64> start_ts_str;
  snprintf(start_ts_str.data(), start_ts_str.size(), "%.6lf", start_ts);
  start_ts_str[start_ts_str.size() - 1] = 0;

  const auto steady_now = std::chrono::steady_clock::now();
  const auto duration = steady_now - start_steady_time_;
  const auto total_time_ms =
      std::chrono::duration_cast<RealMilliseconds>(duration).count();

  const auto& ref_type = GetReferenceType() == ReferenceType::kChild
                             ? kReferenceTypeChild
                             : kReferenceTypeFollows;

  logging::LogExtra result({{kStopWatchAttrName, name_},
                            {kStartTimestampAttrName, start_ts_str.data()},
                            {kTotalTimeAttrName, total_time_ms},
                            {kReferenceType, ref_type},
                            {kTimeUnitsAttrName, "ms"}});

  LOG_INFO() << std::move(result) << std::move(*this);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) const & {
  log_helper << log_extra_inheritable;
  if (log_extra_local) log_helper << log_extra_local.get();
  tracer->LogSpanContextTo(*this, log_helper);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) && {
  log_helper << std::move(log_extra_inheritable);
  if (log_extra_local) log_helper << std::move(log_extra_local.get());
  tracer->LogSpanContextTo(*this, log_helper);
}

const std::string& Span::Impl::GetTraceId() const { return trace_id_; }

const std::string& Span::Impl::GetSpanId() const { return span_id_; }

const std::string& Span::Impl::GetParentId() const { return parent_id_; }

Span::Span(TracerPtr tracer, const std::string& name, const Span* parent,
           ReferenceType reference_type)
    : pimpl_(std::make_unique<Impl>(std::move(tracer), name,
                                    parent ? parent->pimpl_.get() : nullptr,
                                    reference_type)) {}

Span::Span(Span&& other) noexcept = default;

Span::~Span() = default;

Span Span::CreateChild(const std::string& name) const {
  auto span = pimpl_->tracer->CreateSpan(name, *this, ReferenceType::kChild);
  span.pimpl_->log_extra_inheritable = pimpl_->log_extra_inheritable;
  return span;
}

Span Span::CreateFollower(const std::string& name) const {
  auto span =
      pimpl_->tracer->CreateSpan(name, *this, ReferenceType::kReference);
  span.pimpl_->log_extra_inheritable = pimpl_->log_extra_inheritable;
  return span;
}

ScopeTime Span::CreateScopeTime() {
  return ScopeTime(pimpl_->GetTimeStorage());
}

void Span::AddNonInheritableTag(std::string key,
                                logging::LogExtra::Value value) {
  if (!pimpl_->log_extra_local) pimpl_->log_extra_local.emplace();
  pimpl_->log_extra_local->Extend(std::move(key), std::move(value));
}

void Span::AddTag(std::string key, logging::LogExtra::Value value) {
  pimpl_->log_extra_inheritable.Extend(std::move(key), std::move(value));
}

void Span::AddTagFrozen(std::string key, logging::LogExtra::Value value) {
  pimpl_->log_extra_inheritable.Extend(std::move(key), std::move(value),
                                       logging::LogExtra::ExtendType::kFrozen);
}

void Span::SetLink(std::string link) {
  AddTagFrozen(kLinkTag, std::move(link));
}

std::string Span::GetLink() const {
  const auto& value = pimpl_->log_extra_inheritable.GetValue(kLinkTag);
  const auto s = boost::get<std::string>(&value);
  if (s)
    return *s;
  else
    return {};
}

logging::LogExtra& Span::GetInheritableLogExtra() {
  return pimpl_->log_extra_inheritable;
}

void Span::LogTo(logging::LogHelper& log_helper) const & {
  pimpl_->LogTo(log_helper);
}

const std::string& Span::GetTraceId() const { return pimpl_->GetTraceId(); }

const std::string& Span::GetSpanId() const { return pimpl_->GetSpanId(); }

const std::string& Span::GetParentId() const { return pimpl_->GetParentId(); }

}  // namespace tracing

namespace logging {

LogHelper& operator<<(LogHelper& lh, const tracing::Span& span) {
  span.LogTo(lh);
  return lh;
}

LogHelper& operator<<(LogHelper& lh, const tracing::Span::Impl& span_impl) {
  span_impl.LogTo(lh);
  return lh;
}

}  // namespace logging
