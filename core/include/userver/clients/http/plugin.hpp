#pragma once

/// @file userver/clients/http/plugin.hpp
/// @brief @copybrief clients::http::Plugin

#include <chrono>
#include <string>
#include <vector>

#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class RequestState;
class Response;

/// @brief Auxiliary entity that allows editing request to a client
/// from plugins
class PluginRequest final {
 public:
  explicit PluginRequest(RequestState& state);

  void SetHeader(std::string_view name, std::string value);

  void SetTimeout(std::chrono::milliseconds ms);

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

  /// @brief The hook is called before actual HTTP request sending and before
  ///        DNS name resolution. You might want to use the hook for most of the
  ///        hook job.
  virtual void HookPerformRequest(PluginRequest& request) = 0;

  /// @brief The hook is called just after the "external" Span is created.
  ///        You might want to add custom tags from the hook.
  virtual void HookCreateSpan(PluginRequest& request) = 0;

  /// @brief The hook is called after the HTTP response is received or the
  ///        timeout is passed.
  ///
  /// @warning The hook is called in libev thread, not in coroutine context! Do
  ///          not do any heavy work here, offload it to other hooks.
  virtual void HookOnCompleted(PluginRequest& request, Response& response) = 0;

 private:
  const std::string name_;
};

namespace impl {

class PluginPipeline final {
 public:
  PluginPipeline(const std::vector<utils::NotNull<Plugin*>>& plugins);

  void HookPerformRequest(RequestState& request);

  void HookCreateSpan(RequestState& request);

  void HookOnCompleted(RequestState& request, Response& response);

 private:
  const std::vector<utils::NotNull<Plugin*>> plugins_;
};

}  // namespace impl

}  // namespace clients::http

USERVER_NAMESPACE_END
