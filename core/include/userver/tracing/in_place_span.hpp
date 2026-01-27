#pragma once

/// @file userver/tracing/in_place_span.hpp
/// @brief @copybrief tracing::InPlaceSpan

#include <string>

#include <userver/tracing/span.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @brief Avoids an extra allocation by storing tracing::Span data in-place
/// @warning Never put InPlaceSpan on the stack! It has a large size and can
/// cause stack overflow.
class InPlaceSpan final {
public:
    struct DetachedTag {};

    explicit InPlaceSpan(
        std::string&& name,
        const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
    );

    explicit InPlaceSpan(
        std::string&& name,
        std::string_view trace_id,
        std::string_view parent_span_id,
        const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
    );

    explicit InPlaceSpan(
        std::string&& name,
        DetachedTag,
        const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
    );

    InPlaceSpan(InPlaceSpan&&) = delete;
    InPlaceSpan& operator=(InPlaceSpan&&) = delete;
    ~InPlaceSpan();

    const tracing::Span& Get() const noexcept;

    tracing::Span& Get() noexcept;

private:
    struct Impl;
    utils::FastPimpl<Impl, 4392, 8> impl_;
};

}  // namespace tracing

USERVER_NAMESPACE_END
