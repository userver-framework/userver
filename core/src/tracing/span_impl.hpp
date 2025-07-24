#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <boost/intrusive/list.hpp>

#include <userver/logging/level.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/small_string.hpp>

#include <tracing/time_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

inline constexpr std::size_t kTypicalSpanIdSize = 16;
inline constexpr std::size_t kTypicalTraceIdSize = 32;
inline constexpr std::size_t kTypicalLinkSize = 32;

class Span::Impl : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
public:
    Impl(
        std::string name,
        const Impl* parent,
        ReferenceType reference_type,
        const utils::impl::SourceLocation& source_location
    );

    Impl(Impl&&) noexcept = default;

    ~Impl();

    impl::TimeStorage& GetTimeStorage() { return time_storage_; }
    const impl::TimeStorage& GetTimeStorage() const { return time_storage_; }

    // Log this Span specifically
    void PutIntoLogger(logging::impl::TagWriter writer) &&;

    // Add the context of this Span a non-Span-specific log record
    void LogTo(logging::impl::TagWriter writer) const;

    std::string_view GetTraceId() const noexcept { return trace_id_; }
    std::string_view GetSpanId() const noexcept { return span_id_; }
    std::string_view GetParentId() const noexcept { return parent_id_; }
    std::string_view GetLink() const noexcept { return link_; }
    std::string_view GetParentLink() const noexcept { return parent_link_; }
    std::string_view GetName() const noexcept { return name_; }

    void SetTraceId(std::string_view id) noexcept { trace_id_ = std::move(id); }
    void SetSpanId(std::string_view id) noexcept { span_id_ = std::move(id); }
    void SetParentId(std::string_view id) noexcept { parent_id_ = std::move(id); }
    void SetLink(std::string_view id) noexcept { link_ = std::move(id); }
    void SetParentLink(std::string_view id) noexcept { parent_link_ = std::move(id); }

    ReferenceType GetReferenceType() const noexcept { return reference_type_; }

    void DetachFromCoroStack();
    void AttachToCoroStack();

    std::optional<std::string_view> GetSpanIdForChildLogs() const;

private:
    void LogOpenTracing() const;
    void DoLogOpenTracing(const Tracer& tracer, logging::impl::TagWriter writer) const;
    static void AddOpentracingTags(formats::json::StringBuilder& output, const logging::LogExtra& input);

    static std::string_view GetParentIdForLogging(const Span::Impl* parent);
    bool ShouldLog() const;

    const std::string name_;
    const bool is_no_log_span_;
    logging::Level log_level_;
    std::optional<logging::Level> local_log_level_;
    const ReferenceType reference_type_;
    const utils::impl::SourceLocation source_location_;

    Span* span_{nullptr};

    utils::SmallString<kTypicalTraceIdSize> trace_id_;
    utils::SmallString<kTypicalSpanIdSize> span_id_;
    utils::SmallString<kTypicalSpanIdSize> parent_id_;
    utils::SmallString<kTypicalLinkSize> link_;
    utils::SmallString<kTypicalLinkSize> parent_link_;

    logging::LogExtra log_extra_inheritable_;
    std::optional<logging::LogExtra> log_extra_local_;
    impl::TimeStorage time_storage_;

    const std::chrono::system_clock::time_point start_system_time_;
    const std::chrono::steady_clock::time_point start_steady_time_;

    std::vector<SpanEvent> events_;

    friend class Span;
    friend class SpanBuilder;
    friend class TagScope;
};

// Use list instead of stack to avoid UB in case of "pop non-last item"
// in case of buggy users.
using SpanStack = boost::intrusive::list<Span::Impl, boost::intrusive::constant_time_size<false>>;

const Span::Impl* GetParentSpanImpl();

template <typename... Args>
Span::Impl* AllocateImpl(Args&&... args) {
    return new Span::Impl(std::forward<Args>(args)...);
}

}  // namespace tracing

USERVER_NAMESPACE_END
