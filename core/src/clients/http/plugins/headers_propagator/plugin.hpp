#pragma once

#include <userver/clients/http/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

class Plugin final : public http::Plugin {
 public:
  Plugin();

  void HookPerformRequest(PluginRequest&) override;

  void HookCreateSpan(PluginRequest&) override;

  void HookOnCompleted(PluginRequest&, Response&) override;

 private:
  const std::vector<std::string> headers_;
};

}  // namespace clients::http::plugins::headers_propagator

USERVER_NAMESPACE_END
