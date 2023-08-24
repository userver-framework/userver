#pragma once

#include <userver/clients/http/plugin.hpp>
#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::yandex_tracing {

class Plugin final : public http::Plugin {
 public:
  Plugin();

  void HookPerformRequest(PluginRequest& request) override;

  void HookCreateSpan(PluginRequest& request) override;

  void HookOnCompleted(PluginRequest& request, Response& response) override;
};

}  // namespace clients::http::plugins::yandex_tracing

USERVER_NAMESPACE_END
