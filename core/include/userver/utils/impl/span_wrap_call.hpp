#pragma once

/// @file userver/utils/impl/span_wrap_call.hpp
/// @brief Utility functions to start asynchronous tasks.

#include <functional>
#include <string>
#include <utility>

#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/lazy_prvalue.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// A wrapper that obtains a Span from args, attaches it to current coroutine,
// and applies a function to the rest of arguments.
struct SpanWrapCall {
    enum class InheritVariables { kYes, kNo };
    enum class HideSpan { kYes, kNo };

    explicit SpanWrapCall(
        std::string&& name,
        InheritVariables inherit_variables,
        const SourceLocation& location,
        HideSpan hide_span
    );

    SpanWrapCall(const SpanWrapCall&) = delete;
    SpanWrapCall(SpanWrapCall&&) = delete;
    SpanWrapCall& operator=(const SpanWrapCall&) = delete;
    SpanWrapCall& operator=(SpanWrapCall&&) = delete;
    ~SpanWrapCall();

    template <typename Function, typename... Args>
    auto operator()(Function&& f, Args&&... args) {
        DoBeforeInvoke();
        return std::invoke(std::forward<Function>(f), std::forward<Args>(args)...);
    }

private:
    void DoBeforeInvoke();

    struct Impl;

    static constexpr std::size_t kImplSize = 4432;
    static constexpr std::size_t kImplAlign = 8;
    utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

// Note: 'name' and 'location' must outlive the result of this function
inline auto SpanLazyPrvalue(
    std::string&& name,
    SpanWrapCall::InheritVariables inherit_variables = SpanWrapCall::InheritVariables::kYes,
    SpanWrapCall::HideSpan hide_span = SpanWrapCall::HideSpan::kNo,
    const SourceLocation& location = SourceLocation::Current()
) {
    return utils::LazyPrvalue([&name, inherit_variables, &location, hide_span] {
        return SpanWrapCall(std::move(name), inherit_variables, location, hide_span);
    });
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
