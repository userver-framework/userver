#include <tracing/span_impl.hpp>

#include <type_traits>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <engine/task/task_context.hpp>
#include <logging/log_helper_impl.hpp>
#include <userver/engine/task/local_variable.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/tag_writer.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_event.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

using RealMilliseconds = std::chrono::duration<double, std::milli>;

constexpr std::string_view kTraceIdTag = "trace_id";
constexpr std::string_view kSpanIdTag = "span_id";
constexpr std::string_view kParentIdTag = "parent_id";
constexpr std::string_view kLinkTag = "link";
constexpr std::string_view kParentLinkTag = "parent_link";

constexpr std::string_view kStopWatchTag = "stopwatch_name";
constexpr std::string_view kTotalTimeTag = "total_time";
constexpr std::string_view kTimeUnitsTag = "stopwatch_units";
constexpr std::string_view kStartTimestampTag = "start_timestamp";

constexpr std::string_view kReferenceType = "span_ref_type";
constexpr std::string_view kReferenceTypeChild = "child";
constexpr std::string_view kReferenceTypeFollows = "follows";

struct TsBuffer final {
    // digits + dot + fract + (to be sure)
    char data[32]{};
    std::size_t size{};

    std::string_view ToStringView() const& noexcept { return {data, size}; }
    std::string_view ToStringView() && noexcept = delete;
};

TsBuffer StartTsToString(std::chrono::system_clock::time_point start) {
    const auto start_ts_epoch = std::chrono::duration_cast<std::chrono::microseconds>(start.time_since_epoch()).count();
    TsBuffer buffer;
    const auto integral_part = start_ts_epoch / 1000000;
    const auto fractional_part = start_ts_epoch % 1000000;
    buffer.size =
        fmt::format_to_n(
            std::data(buffer.data), std::size(buffer.data), FMT_COMPILE("{}.{:0>6}"), integral_part, fractional_part
        )
            .size;
    return buffer;
}

// Maintain coro-local span stack to identify "current span" in O(1).
engine::TaskLocalVariable<SpanStack> task_local_spans;

template <std::size_t kSboSize, std::size_t kIdSize>
utils::SmallString<kSboSize> GenerateId() {
    using Chunk = utils::RandomBase::result_type;
    static constexpr std::size_t kHexChunkSize = utils::encoding::LengthInHexForm(sizeof(Chunk));
    static_assert(kIdSize % kHexChunkSize == 0);
    static_assert(utils::RandomBase::min() == std::numeric_limits<Chunk>::min());
    static_assert(utils::RandomBase::max() == std::numeric_limits<Chunk>::max());

    utils::SmallString<kSboSize> result;

    result.resize_and_overwrite(kIdSize, [](char* data, std::size_t size) {
        utils::WithDefaultRandom([&](auto& rnd) {
            for (std::size_t pos = 0; pos < size; pos += kHexChunkSize) {
                const auto random_value = rnd();
                utils::encoding::ToHexBuffer(
                    {reinterpret_cast<const char*>(&random_value), sizeof(random_value)}, {data + pos, data + size}
                );
            }
        });
        return size;
    });

    return result;
}

utils::SmallString<kTypicalTraceIdSize> GenerateTraceId() {
    return GenerateId<kTypicalTraceIdSize, opentelemetry::kTraceIdSize>();
}

utils::SmallString<kTypicalSpanIdSize> GenerateSpanId() {
    return GenerateId<kTypicalSpanIdSize, opentelemetry::kSpanIdSize>();
}

utils::SmallString<kTypicalLinkSize> GenerateLink() {
    return GenerateId<kTypicalLinkSize, opentelemetry::kTraceIdSize>();
}

std::string MakeTagFromEvents(const std::vector<SpanEvent>& events) {
    formats::json::StringBuilder builder;
    formats::serialize::WriteToStream(events, builder);
    return builder.GetString();
}

}  // namespace

Span::Impl::Impl(
    std::string name,
    const Impl* parent,
    ReferenceType reference_type,
    const utils::impl::SourceLocation& source_location
)
    : name_(std::move(name)),
      is_no_log_span_(IsNoLogSpan(name_)),
      log_level_(is_no_log_span_ ? logging::Level::kNone : logging::Level::kInfo),
      reference_type_(reference_type),
      source_location_(source_location),
      trace_id_(parent ? utils::SmallString<kTypicalTraceIdSize>(parent->GetTraceId()) : GenerateTraceId()),
      span_id_(GenerateSpanId()),
      parent_id_(GetParentIdForLogging(parent)),
      link_(parent ? utils::SmallString<kTypicalLinkSize>(parent->GetLink()) : GenerateLink()),
      parent_link_(parent ? parent->GetParentLink() : std::string_view{}),
      start_system_time_(std::chrono::system_clock::now()),
      start_steady_time_(std::chrono::steady_clock::now()) {
    if (parent) {
        log_extra_inheritable_ = parent->log_extra_inheritable_;
        local_log_level_ = parent->local_log_level_;
    }
}

Span::Impl::~Impl() {
    if (!ShouldLog()) {
        return;
    }

    {
        const impl::DetachLocalSpansScope ignore_local_span;
        logging::LogHelper lh{logging::GetDefaultLogger(), log_level_, logging::LogClass::kTrace, source_location_};
        std::move(*this).PutIntoLogger(lh.GetTagWriter());
    }
}

void Span::Impl::PutIntoLogger(logging::impl::TagWriter writer) && {
    const auto steady_now = std::chrono::steady_clock::now();
    const auto duration = steady_now - start_steady_time_;
    const auto total_time_ms = std::chrono::duration_cast<RealMilliseconds>(duration).count();
    const auto timestamp_buffer = StartTsToString(start_system_time_);
    const auto ref_type = GetReferenceType() == ReferenceType::kChild ? kReferenceTypeChild : kReferenceTypeFollows;

    writer.PutTag(kTraceIdTag, GetTraceId());
    writer.PutTag(kSpanIdTag, GetSpanId());
    writer.PutTag(kParentIdTag, GetParentId());
    writer.PutTag(kLinkTag, GetLink());
    if (!GetParentLink().empty()) writer.PutTag(kParentLinkTag, GetParentLink());

    writer.PutTag(kStopWatchTag, name_);
    writer.PutTag(kTotalTimeTag, total_time_ms);
    writer.PutTag(kReferenceType, ref_type);
    writer.PutTag(kTimeUnitsTag, "ms");
    writer.PutTag(kStartTimestampTag, timestamp_buffer.ToStringView());

    time_storage_.MergeInto(writer);

    if (log_extra_local_) {
        // TODO apparently, same tags can be added both to log_extra_inheritable_
        //  and log_extra_local_. Merge to deduplicate such tags.
        log_extra_inheritable_.Extend(std::move(*log_extra_local_));
    }
    if (auto& value = log_extra_inheritable_.GetValue(tracing::kSpanKind);
        std::holds_alternative<std::string>(value) && std::get<std::string>(value).empty()) {
        log_extra_inheritable_.Extend(tracing::kSpanKind, tracing::kSpanKindInternal);
    }
    writer.PutLogExtra(log_extra_inheritable_);

    if (!events_.empty()) {
        const auto events_tag = MakeTagFromEvents(events_);
        writer.PutTag("events", events_tag);
    }

    LogOpenTracing();
}

void Span::Impl::LogTo(logging::impl::TagWriter writer) const {
    writer.ExtendLogExtra(log_extra_inheritable_);

    if (const auto span_id = GetSpanIdForChildLogs()) {
        writer.PutTag(kTraceIdTag, GetTraceId());
        writer.PutTag(kSpanIdTag, *span_id);
        writer.PutTag(kLinkTag, GetLink());
    }
}

void Span::Impl::DetachFromCoroStack() { unlink(); }

void Span::Impl::AttachToCoroStack() {
    UASSERT(!is_linked());
    task_local_spans->push_back(*this);
}

std::string_view Span::Impl::GetParentIdForLogging(const Span::Impl* parent) {
    if (!parent) return {};
    return parent->GetSpanIdForChildLogs().value_or(std::string_view{});
}

bool Span::Impl::ShouldLog() const {
    /* We must honour default log level, but use span's level from ourselves,
     * not the previous spans.
     */
    return logging::impl::ShouldLogNoSpan(logging::GetDefaultLogger(), log_level_) &&
           local_log_level_.value_or(logging::Level::kTrace) <= log_level_;
}

std::optional<std::string_view> Span::Impl::GetSpanIdForChildLogs() const {
    if (ShouldLog()) {
        // It's still possible for chaining to break and logs to become orphaned if ShouldLog() becomes false later.
        // TODO set a flag on the current span to force it to be logged in that case?
        return GetSpanId();
    }
    if (!GetParentId().empty()) {
        return GetParentId();
    }
    return std::nullopt;
}

void Span::OptionalDeleter::operator()(Span::Impl* impl) const noexcept {
    if (do_delete) {
        std::default_delete<Impl>{}(impl);
    }
}

Span::OptionalDeleter Span::OptionalDeleter::DoNotDelete() noexcept { return OptionalDeleter{false}; }

Span::OptionalDeleter Span::OptionalDeleter::ShouldDelete() noexcept { return OptionalDeleter(true); }

Span::Span(
    std::string name,
    const Span* parent,
    ReferenceType reference_type,
    const utils::impl::SourceLocation& source_location
)
    : pimpl_(
          AllocateImpl(std::move(name), parent ? parent->pimpl_.get() : nullptr, reference_type, source_location),
          Span::OptionalDeleter{Span::OptionalDeleter::ShouldDelete()}
      ) {
    AttachToCoroStack();
    pimpl_->span_ = this;
}

Span::Span(std::string name, const utils::impl::SourceLocation& source_location)
    : pimpl_(
          AllocateImpl(std::move(name), GetParentSpanImpl(), ReferenceType::kChild, source_location),
          Span::OptionalDeleter{OptionalDeleter::ShouldDelete()}
      ) {
    AttachToCoroStack();
    pimpl_->span_ = this;
}

Span::Span(std::unique_ptr<Span::Impl, OptionalDeleter>&& pimpl) : pimpl_(std::move(pimpl)) { pimpl_->span_ = this; }

Span::Span(Span&& other) noexcept : pimpl_(std::move(other.pimpl_)) { pimpl_->span_ = this; }

Span::~Span() = default;

Span& Span::CurrentSpan() {
    UASSERT_MSG(
        engine::current_task::IsTaskProcessorThread(), "Span::CurrentSpan() called from non coroutine environment"
    );

    auto* span = CurrentSpanUnchecked();
    static constexpr std::string_view msg = "Span::CurrentSpan() called from Span'less task";
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

    const auto* spans_ptr = task_local_spans.GetOptional();
    return !spans_ptr || spans_ptr->empty() ? nullptr : spans_ptr->back().span_;
}

Span Span::MakeSpan(
    std::string name,
    std::string_view trace_id,
    std::string_view parent_span_id,
    const utils::impl::SourceLocation& source_location
) {
    Span span(std::move(name), nullptr, ReferenceType::kChild, source_location);
    if (!trace_id.empty()) span.pimpl_->SetTraceId(trace_id);
    span.pimpl_->SetParentId(parent_span_id);
    return span;
}

Span Span::MakeSpan(
    std::string name,
    std::string_view trace_id,
    std::string_view parent_span_id,
    std::string_view link,
    const utils::impl::SourceLocation& source_location
) {
    Span span(std::move(name), nullptr, ReferenceType::kChild, source_location);
    if (!trace_id.empty()) span.pimpl_->SetTraceId(trace_id);
    span.pimpl_->SetParentId(parent_span_id);
    if (!link.empty()) span.pimpl_->SetLink(link);
    return span;
}

Span Span::MakeRootSpan(std::string name, const utils::impl::SourceLocation& source_location) {
    return Span(std::move(name), nullptr, ReferenceType::kChild, source_location);
}

Span Span::CreateChild(std::string name, const utils::impl::SourceLocation& source_location) const {
    return Span(std::move(name), this, ReferenceType::kChild, source_location);
}

Span Span::CreateFollower(std::string name, const utils::impl::SourceLocation& source_location) const {
    return Span(std::move(name), this, ReferenceType::kReference, source_location);
}

tracing::ScopeTime Span::CreateScopeTime() { return ScopeTime(pimpl_->GetTimeStorage()); }

tracing::ScopeTime Span::CreateScopeTime(std::string name) { return {pimpl_->GetTimeStorage(), std::move(name)}; }

void Span::AddNonInheritableTag(std::string key, logging::LogExtra::Value value) {
    if (!pimpl_->log_extra_local_) pimpl_->log_extra_local_.emplace();
    pimpl_->log_extra_local_->Extend(std::move(key), std::move(value));
}

void Span::AddNonInheritableTags(const logging::LogExtra& log_extra) {
    if (!pimpl_->log_extra_local_) pimpl_->log_extra_local_.emplace();
    pimpl_->log_extra_local_->Extend(log_extra);
}

void Span::SetLogLevel(logging::Level log_level) {
    if (pimpl_->is_no_log_span_) return;
    pimpl_->log_level_ = log_level;
}

logging::Level Span::GetLogLevel() const { return pimpl_->log_level_; }

void Span::SetLocalLogLevel(std::optional<logging::Level> log_level) { pimpl_->local_log_level_ = log_level; }

std::optional<logging::Level> Span::GetLocalLogLevel() const { return pimpl_->local_log_level_; }

void Span::AddTag(std::string key, logging::LogExtra::Value value) {
    pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value));
}

void Span::AddTags(const logging::LogExtra& log_extra, utils::impl::InternalTag) {
    pimpl_->log_extra_inheritable_.Extend(log_extra);
}

impl::TimeStorage& Span::GetTimeStorage(utils::impl::InternalTag) { return pimpl_->GetTimeStorage(); }

void Span::LogTo(utils::impl::InternalTag, logging::impl::TagWriter writer) const { pimpl_->LogTo(writer); }

std::string_view Span::GetTag(std::string_view tag) const {
    const auto& value = pimpl_->log_extra_inheritable_.GetValue(tag);
    const auto* s = std::get_if<std::string>(&value);
    if (s)
        return *s;
    else
        return {};
}

void Span::AddTagFrozen(std::string key, logging::LogExtra::Value value) {
    pimpl_->log_extra_inheritable_.Extend(std::move(key), std::move(value), logging::LogExtra::ExtendType::kFrozen);
}

void Span::AddEvent(std::string_view event_name) { pimpl_->events_.emplace_back(event_name); }

void Span::AddEvent(SpanEvent&& event) { pimpl_->events_.emplace_back(std::move(event)); }

void Span::SetLink(std::string_view link) { pimpl_->SetLink(link); }

void Span::SetParentLink(std::string_view parent_link) { pimpl_->SetParentLink(parent_link); }

bool Span::ShouldLogDefault() const noexcept { return pimpl_->ShouldLog(); }

void Span::DetachFromCoroStack() {
    if (pimpl_) pimpl_->DetachFromCoroStack();
}

void Span::AttachToCoroStack() { pimpl_->AttachToCoroStack(); }

std::chrono::system_clock::time_point Span::GetStartSystemTime() const { return pimpl_->start_system_time_; }

std::string_view Span::GetTraceId() const { return pimpl_->GetTraceId(); }

std::string_view Span::GetSpanId() const { return pimpl_->GetSpanId(); }

std::string_view Span::GetParentId() const { return pimpl_->GetParentId(); }

std::string_view Span::GetLink() const { return pimpl_->GetLink(); }

std::string_view Span::GetParentLink() const { return pimpl_->GetParentLink(); }

std::optional<std::string_view> Span::GetSpanIdForChildLogs() const { return pimpl_->GetSpanIdForChildLogs(); }

std::string_view Span::GetName() const { return pimpl_->GetName(); }

ScopeTime::Duration Span::GetTotalDuration(const std::string& scope_name) const {
    return pimpl_->GetTimeStorage().DurationTotal(scope_name);
}

ScopeTime::DurationMillis Span::GetTotalElapsedTime(const std::string& scope_name) const {
    return std::chrono::duration_cast<ScopeTime::DurationMillis>(GetTotalDuration(scope_name));
}

static_assert(!std::is_copy_assignable_v<Span>, "tracing::Span must not be copy assignable");
static_assert(!std::is_move_assignable_v<Span>, "tracing::Span must not be move assignable");

const Span::Impl* GetParentSpanImpl() {
    if (!engine::current_task::IsTaskProcessorThread()) return nullptr;

    const auto* spans_ptr = task_local_spans.GetOptional();
    return !spans_ptr || spans_ptr->empty() ? nullptr : &spans_ptr->back();
}

namespace impl {

struct DetachLocalSpansScope::Impl {
    SpanStack old_spans;
};

DetachLocalSpansScope::DetachLocalSpansScope() noexcept {
    if (engine::current_task::IsTaskProcessorThread()) {
        if (auto* const spans_ptr = task_local_spans.GetOptional()) {
            impl_->old_spans = std::move(*spans_ptr);
            UASSERT(spans_ptr->empty());
        }
    }
}

DetachLocalSpansScope::~DetachLocalSpansScope() {
    UASSERT_MSG(
        !engine::current_task::IsTaskProcessorThread() || !task_local_spans.GetOptional() || task_local_spans->empty(),
        "A Span was constructed while in DetachLocalSpansScope"
    );
    if (!impl_->old_spans.empty()) {
        *task_local_spans = std::move(impl_->old_spans);
    }
}

logging::LogHelper& operator<<(logging::LogHelper& lh, LogSpanAsLastNoCurrent span) {
    UASSERT(nullptr == Span::CurrentSpanUnchecked());
    span.span.LogTo(utils::impl::InternalTag{}, lh.GetTagWriter());
    return lh;
}

}  // namespace impl

}  // namespace tracing

USERVER_NAMESPACE_END
