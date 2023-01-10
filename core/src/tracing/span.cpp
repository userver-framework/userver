#include <tracing/span_impl.hpp>

#include <random>
#include <type_traits>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <engine/task/task_context.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/uuid4.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

const std::string kLinkTag = "link";
const std::string kParentLinkTag = "parent_link";
using RealMilliseconds = std::chrono::duration<double, std::milli>;

const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";

const std::string kReferenceType = "span_ref_type";
const std::string kReferenceTypeChild = "child";
const std::string kReferenceTypeFollows = "follows";

std::string StartTsToString(std::chrono::system_clock::time_point start) {
  const auto start_ts_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          start.time_since_epoch())
          .count();

  // Avoiding `return fmt::format("{:.6}", float)` because it calls a slow
  // snprintf or gives incorrect results with -DFMT_USE_GRISU=1:
  // 3.1414999961853027 instead of 3.1415

  // TODO: In C++17 with to_chars(float) uncomment the following lines:
  // constexpr std::size_t kBufferSize = 64;
  // std::array<char, kBufferSize> data;
  // auto [out_it, errc] = std::to_chars(data.begin(), data.end(),
  //                    start_ts_epoch * 0.000001, std::chars_format::fixed, 6);
  // UASSERT(errc == 0);
  // return std::string(data.data(), out_it - data.data());

  const auto integral_part = start_ts_epoch / 1000000;
  const auto fractional_part = start_ts_epoch % 1000000;
  return fmt::format(FMT_COMPILE("{}.{:0>6}"), integral_part, fractional_part);
}

/* Maintain coro-local span stack to identify "current span" in O(1).
 * Use list instead of stack to avoid UB in case of "pop non-last item"
 * in case of buggy users.
 */
engine::TaskLocalVariable<boost::intrusive::list<
    Span::Impl, boost::intrusive::constant_time_size<false>>>
    task_local_spans;

std::string GenerateSpanId() {
  std::uniform_int_distribution<std::uint64_t> dist;
  auto random_value = dist(utils::DefaultRandom());
  return fmt::format(FMT_COMPILE("{:016x}"), random_value);
}

logging::LogHelper& operator<<(logging::LogHelper& lh,
                               tracing::Span::Impl&& span_impl) {
  std::move(span_impl).LogTo(lh);
  return lh;
}

const Span::Impl* GetParentSpanImpl() {
  if (!engine::current_task::GetCurrentTaskContextUnchecked()) return nullptr;

  const auto* spans_ptr = task_local_spans.GetOptional();
  return !spans_ptr || spans_ptr->empty() ? nullptr : &spans_ptr->back();
}

}  // namespace

Span::Impl::Impl(std::string name, ReferenceType reference_type,
                 logging::Level log_level)
    : Impl(tracing::Tracer::GetTracer(), std::move(name), GetParentSpanImpl(),
           reference_type, log_level) {}

Span::Impl::Impl(TracerPtr tracer, std::string name, const Span::Impl* parent,
                 ReferenceType reference_type, logging::Level log_level)
    : name_(std::move(name)),
      is_no_log_span_(tracing::Tracer::IsNoLogSpan(name_)),
      log_level_(is_no_log_span_ ? logging::Level::kNone : log_level),
      tracer_(std::move(tracer)),
      start_system_time_(std::chrono::system_clock::now()),
      start_steady_time_(std::chrono::steady_clock::now()),
      trace_id_(parent ? parent->GetTraceId()
                       : utils::generators::GenerateUuid()),
      span_id_(GenerateSpanId()),
      parent_id_(GetParentIdForLogging(parent)),
      reference_type_(reference_type) {
  if (parent) {
    log_extra_inheritable_ = parent->log_extra_inheritable_;
    local_log_level_ = parent->local_log_level_;
  }
}

Span::Impl::~Impl() {
  if (!ShouldLog()) {
    return;
  }

  const auto steady_now = std::chrono::steady_clock::now();
  const auto duration = steady_now - start_steady_time_;
  const auto total_time_ms =
      std::chrono::duration_cast<RealMilliseconds>(duration).count();

  const auto& ref_type = GetReferenceType() == ReferenceType::kChild
                             ? kReferenceTypeChild
                             : kReferenceTypeFollows;

  logging::LogExtra result;

  // Using result.Extend to move construct the keys and values.
  result.Extend(kStopWatchAttrName, name_);
  result.Extend(kTotalTimeAttrName, total_time_ms);
  result.Extend(kReferenceType, ref_type);
  result.Extend(kTimeUnitsAttrName, "ms");
  result.Extend(kStartTimestampAttrName, StartTsToString(start_system_time_));

  LogOpenTracing();

  if (log_extra_local_) result.Extend(std::move(*log_extra_local_));
  time_storage_.MergeInto(result);

  DO_LOG_TO_NO_SPAN(logging::DefaultLogger(), log_level_)
      << std::move(result) << std::move(*this);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) const& {
  log_helper << log_extra_inheritable_;
  tracer_->LogSpanContextTo(*this, log_helper);
}

void Span::Impl::LogTo(logging::LogHelper& log_helper) && {
  log_helper << std::move(log_extra_inheritable_);
  tracer_->LogSpanContextTo(std::move(*this), log_helper);
}

void Span::Impl::DetachFromCoroStack() { unlink(); }

void Span::Impl::AttachToCoroStack() {
  UASSERT(!is_linked());
  task_local_spans->push_back(*this);
  UINVARIANT(!GetSpansStackFromContext(
                  engine::current_task::GetCurrentTaskContextUnchecked())
                  .empty(),
             "");
}

std::vector<const Span::Impl*> Span::Impl::GetSpansStackFromContext(
    engine::impl::TaskContext* context) {
  UASSERT(context);
  UASSERT(context->HasLocalStorage());

  const auto* spans_stack =
      context->GetLocalStorage()
          .GetOptional<decltype(task_local_spans)::VariableType,
                       engine::impl::task_local::VariableKind::kNormal>(
              task_local_spans.GetKey());
  if (!spans_stack || spans_stack->empty()) {
    return {};
  }

  std::vector<const Span::Impl*> result{};
  for (const auto& span_impl : *spans_stack) {
    result.push_back(&span_impl);
  }
  std::reverse(result.begin(), result.end());

  return result;
}

std::string Span::Impl::GetParentIdForLogging(const Span::Impl* parent) {
  if (!parent) return {};

  if (!parent->is_linked()) {
    return parent->GetSpanId();
  }

  const auto* spans_ptr = task_local_spans.GetOptional();

  // No parents
  if (!spans_ptr) return {};

  // Should find the closest parent that is loggable at the moment,
  // otherwise span_id -> parent_id chaining might break and some spans become
  // orphaned. It's still possible for chaining to break in case parent span
  // becomes non-loggable after child span is created, but that we can't control
  for (auto current = spans_ptr->iterator_to(*parent);; --current) {
    if (current->GetParentId().empty() /* won't find better candidate */ ||
        current->ShouldLog()) {
      return current->GetSpanId();
    }
    if (current == spans_ptr->begin()) break;
  };

  return {};
}

bool Span::Impl::ShouldLog() const {
  /* We must honour default log level, but use span's level from ourselves,
   * not the previous span's.
   */
  return logging::ShouldLogNospan(log_level_) &&
         local_log_level_.value_or(logging::Level::kTrace) <= log_level_;
}

namespace {
template <typename... Args>
Span::Impl* AllocateImpl(Args&&... args) {
  return new Span::Impl(std::forward<Args>(args)...);
}
}  // namespace

void Span::OptionalDeleter::operator()(Span::Impl* impl) const noexcept {
  if (do_delete) {
    std::default_delete<Impl>{}(impl);
  }
}

Span::OptionalDeleter Span::OptionalDeleter::DoNotDelete() noexcept {
  return OptionalDeleter{false};
}

Span::OptionalDeleter Span::OptionalDeleter::ShouldDelete() noexcept {
  return OptionalDeleter(true);
}

Span::Span(TracerPtr tracer, std::string name, const Span* parent,
           ReferenceType reference_type, logging::Level log_level)
    : pimpl_(AllocateImpl(std::move(tracer), std::move(name),
                          parent ? parent->pimpl_.get() : nullptr,
                          reference_type, log_level),
             Span::OptionalDeleter{Span::OptionalDeleter::ShouldDelete()}) {
  AttachToCoroStack();
  pimpl_->span_ = this;
}

Span::Span(std::string name, ReferenceType reference_type,
           logging::Level log_level)
    : pimpl_(AllocateImpl(tracing::Tracer::GetTracer(), std::move(name),
                          GetParentSpanImpl(), reference_type, log_level),
             Span::OptionalDeleter{OptionalDeleter::ShouldDelete()}) {
  AttachToCoroStack();
  if (pimpl_->GetParentId().empty()) {
    SetLink(utils::generators::GenerateUuid());
  }
  pimpl_->span_ = this;
}

Span::Span(Span::Impl& impl)
    : pimpl_(&impl, Span::OptionalDeleter{OptionalDeleter::DoNotDelete()}) {
  pimpl_->span_ = this;
}

Span::Span(Span&& other) noexcept : pimpl_(std::move(other.pimpl_)) {
  pimpl_->span_ = this;
}

Span::~Span() = default;

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
  auto* current = engine::current_task::GetCurrentTaskContextUnchecked();
  if (current == nullptr) return nullptr;
  if (!current->HasLocalStorage()) return nullptr;

  const auto* spans_ptr = task_local_spans.GetOptional();
  return !spans_ptr || spans_ptr->empty() ? nullptr : spans_ptr->back().span_;
}

Span Span::MakeSpan(std::string name, std::string_view trace_id,
                    std::string_view parent_span_id) {
  Span span(std::move(name));
  if (!trace_id.empty()) span.pimpl_->SetTraceId(std::string{trace_id});
  span.pimpl_->SetParentId(std::string{parent_span_id});
  return span;
}

Span Span::MakeSpan(std::string name, std::string_view trace_id,
                    std::string_view parent_span_id, std::string link) {
  Span span(Tracer::GetTracer(), std::move(name), nullptr,
            ReferenceType::kChild);
  span.SetLink(std::move(link));
  if (!trace_id.empty()) span.pimpl_->SetTraceId(std::string{trace_id});
  span.pimpl_->SetParentId(std::string{parent_span_id});
  return span;
}

Span Span::CreateChild(std::string name) const {
  auto span = pimpl_->tracer_->CreateSpan(std::move(name), *this,
                                          ReferenceType::kChild);
  return span;
}

Span Span::CreateFollower(std::string name) const {
  auto span = pimpl_->tracer_->CreateSpan(std::move(name), *this,
                                          ReferenceType::kReference);
  return span;
}

tracing::ScopeTime Span::CreateScopeTime() {
  return ScopeTime(pimpl_->GetTimeStorage());
}

tracing::ScopeTime Span::CreateScopeTime(std::string name) {
  return {pimpl_->GetTimeStorage(), std::move(name)};
}

void Span::AddNonInheritableTag(std::string key,
                                logging::LogExtra::Value value) {
  if (!pimpl_->log_extra_local_) pimpl_->log_extra_local_.emplace();
  pimpl_->log_extra_local_->Extend(std::move(key), std::move(value));
}

void Span::SetLogLevel(logging::Level log_level) {
  if (pimpl_->is_no_log_span_) return;
  pimpl_->log_level_ = log_level;
}

logging::Level Span::GetLogLevel() const { return pimpl_->log_level_; }

void Span::SetLocalLogLevel(std::optional<logging::Level> log_level) {
  pimpl_->local_log_level_ = log_level;
}

std::optional<logging::Level> Span::GetLocalLogLevel() const {
  return pimpl_->local_log_level_;
}

void Span::AddTag(std::string key, logging::LogExtra::Value value) {
  pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value));
}

void Span::AddTags(const logging::LogExtra& log_extra, utils::InternalTag) {
  pimpl_->log_extra_inheritable_.Extend(log_extra);
}

impl::TimeStorage& Span::GetTimeStorage() { return pimpl_->GetTimeStorage(); }

std::string Span::GetTag(std::string_view tag) const {
  const auto& value = pimpl_->log_extra_inheritable_.GetValue(tag);
  const auto* s = std::get_if<std::string>(&value);
  if (s)
    return *s;
  else
    return {};
}

void Span::AddTagFrozen(std::string key, logging::LogExtra::Value value) {
  pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value),
                                        logging::LogExtra::ExtendType::kFrozen);
}

void Span::SetLink(std::string link) {
  AddTagFrozen(kLinkTag, std::move(link));
}

void Span::SetParentLink(std::string parent_link) {
  AddTagFrozen(kParentLinkTag, std::move(parent_link));
}

std::string Span::GetLink() const { return GetTag(kLinkTag); }

std::string Span::GetParentLink() const { return GetTag(kParentLinkTag); }

void Span::LogTo(logging::LogHelper& log_helper) const& {
  pimpl_->LogTo(log_helper);
}

bool Span::ShouldLogDefault() const noexcept { return pimpl_->ShouldLog(); }

void Span::DetachFromCoroStack() {
  if (pimpl_) pimpl_->DetachFromCoroStack();
}

void Span::AttachToCoroStack() { pimpl_->AttachToCoroStack(); }

const std::string& Span::GetTraceId() const { return pimpl_->GetTraceId(); }

const std::string& Span::GetSpanId() const { return pimpl_->GetSpanId(); }

const std::string& Span::GetParentId() const { return pimpl_->GetParentId(); }

ScopeTime::Duration Span::GetTotalDuration(
    const std::string& scope_name) const {
  return pimpl_->GetTimeStorage().DurationTotal(scope_name);
}

ScopeTime::DurationMillis Span::GetTotalElapsedTime(
    const std::string& scope_name) const {
  return std::chrono::duration_cast<ScopeTime::DurationMillis>(
      GetTotalDuration(scope_name));
}

static_assert(!std::is_copy_assignable<Span>::value,
              "tracing::Span must not be copy assignable");
static_assert(!std::is_move_assignable<Span>::value,
              "tracing::Span must not be move assignable");

logging::LogHelper& operator<<(logging::LogHelper& lh,
                               const tracing::Span& span) {
  span.LogTo(lh);
  return lh;
}

}  // namespace tracing

USERVER_NAMESPACE_END
