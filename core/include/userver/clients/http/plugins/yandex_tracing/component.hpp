#pragma once

#include <memory>

#include <userver/clients/http/plugin_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::yandex_tracing {

class Plugin;

class Component final : public plugin::ComponentBase {
 public:
  static constexpr auto kName = "http-client-plugin-yandex-tracing";

  Component(const components::ComponentConfig&,
            const components::ComponentContext&);

  ~Component() override;

  http::Plugin& GetPlugin() override;

 private:
  std::unique_ptr<yandex_tracing::Plugin> plugin_;
};

}  // namespace clients::http::plugins::yandex_tracing

USERVER_NAMESPACE_END
