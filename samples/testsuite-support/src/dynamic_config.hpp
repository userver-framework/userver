#pragma once

#include <userver/dynamic_config/source.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace tests::handlers {

class DynamicConfig final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-dynamic-config";

  DynamicConfig(const components::ComponentConfig&,
                const components::ComponentContext&);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

 private:
  dynamic_config::Source config_source_;
};

}  // namespace tests::handlers
