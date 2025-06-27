#pragma once

/// @file userver/clients/http/plugin.hpp
/// @brief @copybrief clients::http::Plugin

#include <chrono>
#include <string>
#include <system_error>
#include <vector>

#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}

namespace clients::http {

class RequestState;
class Response;

/// @brief Auxiliary entity that allows editing request to a client
/// from plugins
class PluginRequest final {
public:
    /// @cond
    explicit PluginRequest(RequestState& state);
    /// @endcond

    void SetHeader(std::string_view name, std::string_view value);

    void AddQueryParams(std::string_view params);

    void SetTimeout(std::chrono::milliseconds ms);

    const std::string& GetOriginalUrl() const;

private:
    RequestState& state_;
};

/// @brief Base class for HTTP Client plugins
class Plugin {
public:
    explicit Plugin(std::string name);

    virtual ~Plugin() = default;

    /// @brief Get plugin name
    const std::string& GetName() const;

    /// @brief The hook is called just after the "external" Span is created.
    ///        You might want to add custom tags from the hook.
    ///        The hook is called only once before any network interaction and is NOT called before each retry.
    ///        The hook is executed in the context of the parent task which created the request.
    virtual void HookCreateSpan(PluginRequest& request, tracing::Span& span) = 0;

    /// @brief The hook is called before actual HTTP request sending and before
    ///        DNS name resolution.
    ///        This hook is called on each retry.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookPerformRequest(PluginRequest& request) = 0;

    /// @brief The hook is called after the HTTP response is received. It does not
    ///        include timeout, network or other error.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookOnCompleted(PluginRequest& request, Response& response) = 0;

    /// @brief The hook is called on timeout and other errors. The hook is not called
    ///        for error HTTP responses, use HookOnCompleted instead.
    ///
    /// @warning The hook is called in libev thread, not in coroutine context! Do
    ///          not do any heavy work here, offload it to other hooks.
    virtual void HookOnError(PluginRequest& request, std::error_code ec) = 0;

    /// @brief The hook is called before every call attempt except the first one.
    /// @return whether the retry is allowed.
    virtual bool HookOnRetry(PluginRequest& request) = 0;

private:
    const std::string name_;
};

namespace impl {

class PluginPipeline final {
public:
    PluginPipeline(const std::vector<utils::NotNull<Plugin*>>& plugins);

    void HookPerformRequest(RequestState& request);

    void HookCreateSpan(RequestState& request, tracing::Span& span);

    void HookOnCompleted(RequestState& request, Response& response);

    void HookOnError(RequestState& request, std::error_code ec);

    bool HookOnRetry(RequestState& request);

private:
    const std::vector<utils::NotNull<Plugin*>> plugins_;
};

}  // namespace impl

}  // namespace clients::http

USERVER_NAMESPACE_END
