#include <tracing/span.hpp>
#include <tracing/span_impl.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/format.hpp>

#include <type_traits>

#include <engine/task/local_variable.hpp>
#include <engine/task/task_context.hpp>
#include <tracing/tracer.hpp>
#include <utils/assert.hpp>
#include <utils/uuid4.hpp>

namespace tracing {

namespace {

const std::string kLinkTag = "link";
using RealMilliseconds = std::chrono::duration<double, std::milli>;

const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTotalTimeSecondsAttrName = "delay";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";

const std::string kReferenceType = "span_ref_type";
const std::string kReferenceTypeChild = "child";
const std::string kReferenceTypeFollows = "follows";

/* Maintain coro-local span stack to identify "current span" in O(1).
 * Use list instead of stack to avoid UB in case of "pop non-last item"
 * in case of buggy users.
 */
engine::TaskLocalVariable<boost::intrusive::list<
    Span::Impl, boost::intrusive::constant_time_size<false>>>
    task_local_spans;

}  // namespace

Span::Impl::Impl(TracerPtr tracer, const std::string& name,
                 const Span::Impl* parent, ReferenceType reference_type,
                 logging::Level log_level)
    : log_level_(logging::ShouldLog(log_level) ? log_level
                                               : logging::Level::kNone),
      tracer(std::move(tracer)),
      name_(name),
      start_system_time_(log_level == logging::Level::kNone
                             ? std::chrono::system_clock::time_point::min()
                             : std::chrono::system_clock::now()),
      start_steady_time_(log_level == logging::Level::kNone
                             ? std::chrono::steady_clock::time_point::min()
                             : std::chrono::steady_clock::now()),
      trace_id_(parent ? parent->GetTraceId()
                       : utils::generators::GenerateUuid()),
      span_id_(utils::generators::GenerateUuid()),
      parent_id_(parent ? parent->GetSpanId() : std::string{}),
      reference_type_(reference_type) {
  if (parent) log_extra_inheritable = parent->log_extra_inheritable;
  AttachToCoroStack();
}

Span::Impl::~Impl() {
  if (!logging::ShouldLog(log_level_)) {
    return;
  }

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
                            {kTotalTimeSecondsAttrName, total_time_ms / 1000.0},
                            {kReferenceType, ref_type},
                            {kTimeUnitsAttrName, "ms"}});

  if (log_extra_local_) result.Extend(std::move(*log_extra_local_));

  if (time_storage_) {
    result.Extend(time_storage_->GetLogs());
  }

  LOG(log_level_) << std::move(result) << std::move(*this);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) const& {
  log_helper << log_extra_inheritable;
  tracer->LogSpanContextTo(*this, log_helper);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) && {
  log_helper << std::move(log_extra_inheritable);
  tracer->LogSpanContextTo(*this, log_helper);
}

const std::string& Span::Impl::GetTraceId() const { return trace_id_; }

const std::string& Span::Impl::GetSpanId() const { return span_id_; }

const std::string& Span::Impl::GetParentId() const { return parent_id_; }

void Span::Impl::DetachFromCoroStack() { unlink(); }

void Span::Impl::AttachToCoroStack() {
  UASSERT(!is_linked());
  task_local_spans->push_back(*this);
}

Span::Span(TracerPtr tracer, const std::string& name, const Span* parent,
           ReferenceType reference_type, logging::Level log_level)
    : pimpl_(std::make_unique<Impl>(std::move(tracer), name,
                                    parent ? parent->pimpl_.get() : nullptr,
                                    reference_type, log_level)) {}

Span::Span(const std::string& name, ReferenceType reference_type,
           logging::Level log_level)
    : pimpl_(std::make_unique<Impl>(
          tracing::Tracer::GetTracer(), name,
          task_local_spans->empty() ? nullptr : &task_local_spans->back(),
          reference_type, log_level)) {
  if (pimpl_->parent_id_.empty()) {
    SetLink(utils::generators::GenerateUuid());
  }
  pimpl_->span_ = this;
}

Span::Span(Span&& other) noexcept : pimpl_(std::move(other.pimpl_)) {
  pimpl_->span_ = this;
}

Span::~Span() { DetachFromCoroStack(); }

Span& Span::CurrentSpan() {
  auto* span = CurrentSpanUnchecked();
  UASSERT(span != nullptr);
  if (span == nullptr) {
    static constexpr const char* msg =
        "Span::CurrentSpan() called from Span'less task";
    LOG_ERROR() << msg << logging::LogExtra::Stacktrace();
    throw std::logic_error(msg);
  }
  return *span;
}

Span* Span::CurrentSpanUnchecked() {
  auto current = engine::current_task::GetCurrentTaskContextUnchecked();
  if (current == nullptr) return nullptr;
  if (!current->HasLocalStorage()) return nullptr;
  return task_local_spans->empty() ? nullptr : task_local_spans->back().span_;
}

Span Span::MakeSpan(const std::string& name, const std::string& trace_id,
                    const std::string& parent_span_id) {
  Span span(name);
  if (!trace_id.empty()) span.pimpl_->trace_id_ = trace_id;
  span.pimpl_->parent_id_ = parent_span_id;
  return span;
}

Span Span::CreateChild(const std::string& name) const {
  auto span = pimpl_->tracer->CreateSpan(name, *this, ReferenceType::kChild);
  return span;
}

Span Span::CreateFollower(const std::string& name) const {
  auto span =
      pimpl_->tracer->CreateSpan(name, *this, ReferenceType::kReference);
  return span;
}

ScopeTime Span::CreateScopeTime() {
  return ScopeTime(pimpl_->GetTimeStorage());
}

ScopeTime Span::CreateScopeTime(const std::string& name) {
  return ScopeTime(pimpl_->GetTimeStorage(), name);
}

void Span::AddNonInheritableTag(std::string key,
                                logging::LogExtra::Value value) {
  if (!pimpl_->log_extra_local_) pimpl_->log_extra_local_.emplace();
  pimpl_->log_extra_local_->Extend(std::move(key), std::move(value));
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

void Span::LogTo(logging::LogHelper& log_helper) const& {
  pimpl_->LogTo(log_helper);
}

void Span::DetachFromCoroStack() {
  if (pimpl_) pimpl_->DetachFromCoroStack();
}

void Span::AttachToCoroStack() { pimpl_->AttachToCoroStack(); }

const std::string& Span::GetTraceId() const { return pimpl_->GetTraceId(); }

const std::string& Span::GetSpanId() const { return pimpl_->GetSpanId(); }

const std::string& Span::GetParentId() const { return pimpl_->GetParentId(); }

static_assert(!std::is_copy_assignable<Span>::value,
              "tracing::Span must not be copy assignable");
static_assert(!std::is_move_assignable<Span>::value,
              "tracing::Span must not be move assignable");

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
