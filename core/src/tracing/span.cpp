#include <tracing/span_impl.hpp>

#include <random>
#include <type_traits>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <engine/task/task_context.hpp>
#include <logging/put_data.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/uuid4.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

using RealMilliseconds = std::chrono::duration<double, std::milli>;

const std::string kStopWatchAttrName = "stopwatch_name";
const std::string kTotalTimeAttrName = "total_time";
const std::string kTimeUnitsAttrName = "stopwatch_units";
const std::string kStartTimestampAttrName = "start_timestamp";

const std::string kReferenceType = "span_ref_type";
const std::string kReferenceTypeChild = "child";
const std::string kReferenceTypeFollows = "follows";

std::string_view StartTsToString(std::chrono::system_clock::time_point start) {
  const auto start_ts_epoch =
      std::chrono::duration_cast<std::chrono::microseconds>(
          start.time_since_epoch())
          .count();
  // digits + dot + fract + (to be sure)
  thread_local char buffer[32];

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
  auto size = fmt::format_to_n(buffer, sizeof(buffer), FMT_COMPILE("{}.{:0>6}"),
                               integral_part, fractional_part);
  return std::string_view{&buffer[0], size.size};
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

  static_assert(sizeof(random_value) == 8);
  return utils::encoding::ToHex(&random_value, 8);
}

logging::LogHelper& operator<<(logging::LogHelper& lh,
                               tracing::Span::Impl&& span_impl) {
  std::move(span_impl).LogTo(lh);
  return lh;
}

}  // namespace

Span::Impl::Impl(std::string name, ReferenceType reference_type,
                 logging::Level log_level,
                 utils::impl::SourceLocation source_location)
    : Impl(tracing::Tracer::GetTracer(), std::move(name), GetParentSpanImpl(),
           reference_type, log_level, source_location) {}

Span::Impl::Impl(TracerPtr tracer, std::string name, const Span::Impl* parent,
                 ReferenceType reference_type, logging::Level log_level,
                 utils::impl::SourceLocation source_location)
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
      reference_type_(reference_type),
      source_location_(source_location) {
  if (parent) {
    log_extra_inheritable_ = parent->log_extra_inheritable_;
    local_log_level_ = parent->local_log_level_;
  }
}

Span::Impl::~Impl() {
  if (!ShouldLog()) {
    return;
  }

  const auto* file_path = source_location_.file_name();

  PutIntoLogger(logging::LogHelper(
                    logging::impl::DefaultLoggerRef(), log_level_, file_path,
                    source_location_.line(), source_location_.function_name(),
                    logging::LogHelper::Mode::kNoSpan)
                    .AsLvalue());
}

void Span::Impl::PutIntoLogger(logging::LogHelper& lh) {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto duration = steady_now - start_steady_time_;
  const auto total_time_ms =
      std::chrono::duration_cast<RealMilliseconds>(duration).count();

  const auto& ref_type = GetReferenceType() == ReferenceType::kChild
                             ? kReferenceTypeChild
                             : kReferenceTypeFollows;

  PutData(lh, kStopWatchAttrName, name_);
  PutData(lh, kTotalTimeAttrName, total_time_ms);
  PutData(lh, kReferenceType, ref_type);
  PutData(lh, kTimeUnitsAttrName, "ms");
  PutData(lh, kStartTimestampAttrName, StartTsToString(start_system_time_));

  LogOpenTracing();

  time_storage_.MergeInto(lh);

  if (log_extra_local_) lh << std::move(*log_extra_local_);
  lh << std::move(*this);
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
           ReferenceType reference_type, logging::Level log_level,
           utils::impl::SourceLocation source_location)
    : pimpl_(AllocateImpl(std::move(tracer), std::move(name),
                          parent ? parent->pimpl_.get() : nullptr,
                          reference_type, log_level, source_location),
             Span::OptionalDeleter{Span::OptionalDeleter::ShouldDelete()}) {
  AttachToCoroStack();
  pimpl_->span_ = this;
}

Span::Span(std::string name, ReferenceType reference_type,
           logging::Level log_level,
           utils::impl::SourceLocation source_location)
    : pimpl_(AllocateImpl(tracing::Tracer::GetTracer(), std::move(name),
                          GetParentSpanImpl(), reference_type, log_level,
                          source_location),
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

Span::Span(std::unique_ptr<Span::Impl, OptionalDeleter>&& pimpl)
    : pimpl_(std::move(pimpl)) {
  pimpl_->span_ = this;
}

Span::Span(Span&& other) noexcept : pimpl_(std::move(other.pimpl_)) {
  pimpl_->span_ = this;
}

Span::~Span() = default;

Span& Span::CurrentSpan() {
  UASSERT_MSG(engine::current_task::IsTaskProcessorThread(),
              "Span::CurrentSpan() called from non coroutine environment");

  auto* span = CurrentSpanUnchecked();
  static constexpr std::string_view msg =
      "Span::CurrentSpan() called from Span'less task";
  UASSERT_MSG(span != nullptr, msg);
  if (span == nullptr) {
    LOG_ERROR() << msg << logging::LogExtra::Stacktrace();
    throw std::logic_error(std::string{msg});
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

std::chrono::system_clock::time_point Span::GetStartSystemTime() const {
  return pimpl_->start_system_time_;
}

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

const Span::Impl* GetParentSpanImpl() {
  if (!engine::current_task::IsTaskProcessorThread()) return nullptr;

  const auto* spans_ptr = task_local_spans.GetOptional();
  return !spans_ptr || spans_ptr->empty() ? nullptr : &spans_ptr->back();
}

}  // namespace tracing

USERVER_NAMESPACE_END
