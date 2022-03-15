#pragma once

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace tests::handlers {

class Now final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-now";

  Now(const components::ComponentConfig& config,
      const components::ComponentContext& component_context)
      : server::handlers::HttpHandlerJsonBase(config, component_context) {}

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_body,
      server::request::RequestContext& context) const override;
};

}  // namespace tests::handlers
