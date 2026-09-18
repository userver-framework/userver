#pragma once

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/http/predefined_header.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

/// @brief Event-like helper for waiting until HTTP headers can be sent.
///
/// For streamed responses this happens on the first body chunk (or when the
/// handler finishes). It does not wait for the full response body.
class HeadersEndEvent final {
public:
    explicit HeadersEndEvent(engine::SingleConsumerEvent& event) noexcept : event_(event) {}

    [[nodiscard]] bool IsReady() const noexcept { return event_.IsReady(); }

    /// @brief Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAny.
    [[nodiscard]] engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return event_.GetAwaitableToken();
    }

    /// @returns whether headers became ready before deadline or cancellation.
    [[nodiscard]] bool Wait() { return event_.WaitForEvent(); }

private:
    engine::SingleConsumerEvent& event_;
};

void OutputHeader(USERVER_NAMESPACE::http::headers::HeadersString& header, std::string_view key, std::string_view val);

}  // namespace server::http::impl

USERVER_NAMESPACE_END
