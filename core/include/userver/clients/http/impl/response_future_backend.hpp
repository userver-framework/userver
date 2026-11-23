#pragma once

/// @file userver/clients/http/impl/response_future_backend.hpp
/// @brief @copybrief clients::http::impl::ResponseFutureBackend

#include <future>
#include <memory>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class Response;

}  // namespace clients::http

namespace clients::http::impl {

/// @brief Extension point adapting an asynchronous HTTP operation to @ref clients::http::ResponseFuture.
///
/// The implementation owns its native future and converts its result in
/// @ref clients::http::impl::ResponseFutureBackend::Get.
/// This interface requires no extra coroutine or engine promise.
/// Preserve the @ref clients::http::ResponseFuture cancellation and deadline contracts: destruction
/// cancels or detaches according to the operation's cancellation policy, and a
/// detached operation retains its transport dependencies until completion.
/// Cancellation must stop further retries. Map native errors to the
/// @ref clients::http::BaseException hierarchy. Waiting must suspend the coroutine
/// rather than block its task processor and must observe task cancellation.
/// Cancel and Wait must not be called concurrently on the same operation.
class ResponseFutureBackend {
public:
    virtual ~ResponseFutureBackend();

    /// @brief Cancels the operation and detaches this result; repeated calls are harmless.
    /// Must retain awaitable state until this backend is destroyed.
    virtual void Cancel() = 0;

    /// @brief Relinquishes the result without cancelling the operation; repeated calls are harmless.
    /// Must retain awaitable state until this backend is destroyed.
    virtual void Detach() = 0;

    /// @brief Waits until completion or the operation's wait deadline.
    /// Throws @ref clients::http::CancelException on task cancellation and std::logic_error after detachment.
    virtual std::future_status Wait(utils::impl::SourceLocation location) = 0;

    /// @brief Returns whether the result is ready, without waiting or extracting it.
    virtual bool IsReady() const = 0;

    /// @brief Waits and extracts the response, or throws the mapped request failure.
    /// On success, consumes the result and detaches the operation.
    /// Throws @ref clients::http::TimeoutException on wait timeout and std::logic_error after detachment.
    virtual std::shared_ptr<Response> Get(utils::impl::SourceLocation location) = 0;

    /// @brief Returns the completion token used by @ref engine::WaitAnyContext.
    /// Previously returned tokens remain valid through Cancel/Detach until backend destruction.
    virtual engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND = 0;
};

}  // namespace clients::http::impl

USERVER_NAMESPACE_END
