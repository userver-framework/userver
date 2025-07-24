#include <userver/tracing/in_place_span.hpp>

#include <utility>

#include <tracing/span_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

struct InPlaceSpan::Impl final {
    template <typename... Args>
    explicit Impl(Args&&... args)
        : span_impl(std::forward<Args>(args)...),
          span(std::unique_ptr<Span::Impl, Span::OptionalDeleter>(&span_impl, Span::OptionalDeleter::DoNotDelete())) {}

    Span::Impl span_impl;
    Span span;
};

InPlaceSpan::InPlaceSpan(std::string&& name, const utils::impl::SourceLocation& source_location)
    : impl_(std::move(name), GetParentSpanImpl(), ReferenceType::kChild, source_location) {
    impl_->span.AttachToCoroStack();
}

InPlaceSpan::InPlaceSpan(
    std::string&& name,
    std::string_view trace_id,
    std::string_view parent_span_id,
    const utils::impl::SourceLocation& source_location
)
    : impl_(std::move(name), GetParentSpanImpl(), ReferenceType::kChild, source_location) {
    impl_->span.AttachToCoroStack();
    impl_->span_impl.SetTraceId(trace_id);
    impl_->span_impl.SetParentId(parent_span_id);
}

InPlaceSpan::InPlaceSpan(std::string&& name, DetachedTag, const utils::impl::SourceLocation& source_location)
    : impl_(std::move(name), GetParentSpanImpl(), ReferenceType::kChild, source_location) {}

InPlaceSpan::~InPlaceSpan() = default;

const tracing::Span& InPlaceSpan::Get() const noexcept { return impl_->span; }

tracing::Span& InPlaceSpan::Get() noexcept { return impl_->span; }

}  // namespace tracing

USERVER_NAMESPACE_END
