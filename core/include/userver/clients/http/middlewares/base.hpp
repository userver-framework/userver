#pragma once

/// @file userver/clients/http/middlewares/base.hpp
/// @brief @copybrief clients::http::MiddlewareBase

#include <chrono>
#include <string>
#include <system_error>

#include <userver/utils/not_null.hpp>
#include <userver/utils/span.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}

namespace clients::http {

class RequestState;
class Response;

/// @brief Auxiliary entity that allows editing request to a client
/// from middlewares
class MiddlewareRequest final {
public:
    /// @cond
    explicit MiddlewareRequest(RequestState& state);
    /// @endcond

    void SetHeader(std::string_view name, std::string_view value);

    void AddQueryParams(std::string_view params);

    void SetTimeout(std::chrono::milliseconds ms);

    void SetProxy(utils::zstring_view value);

    bool IsProxySet() const;

    const std::string& GetOriginalUrl() const;

private:
    RequestState& state_;
};

/// @brief Base class for HTTP Client middlewares
class MiddlewareBase {
public:
    virtual ~MiddlewareBase();

    MiddlewareBase(const MiddlewareBase&) = delete;
    MiddlewareBase(MiddlewareBase&&) = delete;

    MiddlewareBase& operator=(const MiddlewareBase&) = delete;
    MiddlewareBase& operator=(MiddlewareBase&&) = delete;

    /// @brief The hook is called just after the "external" Span is created.
    ///        You might want to add custom tags from the hook.
    ///        The hook is called only once before any network interaction and is NOT called before each retry.
    ///        The hook is executed in the context of the parent task which created the request.
    ///        The hook does nothing by default.
    virtual void HookCreateSpan(MiddlewareRequest& request, tracing::Span& span);

    /// @brief The hook is called before actual HTTP request sending and before
    ///        DNS name resolution.
    ///        This hook is called on each retry.
    ///        The hook does nothing by default.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookPerformRequest(MiddlewareRequest& request);

    /// @brief The hook is called after the HTTP response is received. It does not
    ///        include timeout, network or other error.
    ///        The hook does nothing by default.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookOnCompleted(MiddlewareRequest& request, Response& response);

    /// @brief The hook is called on timeout and other errors. The hook is not called
    ///        for error HTTP responses, use HookOnCompleted instead.
    ///        The hook does nothing by default.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookOnError(MiddlewareRequest& request, std::error_code ec);

    /// @brief The hook is called before every call attempt except the first one.
    ///        The hook allows retry by default.
    /// @return whether the retry is allowed.
    virtual bool HookOnRetry(MiddlewareRequest& request);

protected:
    MiddlewareBase();
};

}  // namespace clients::http

USERVER_NAMESPACE_END
