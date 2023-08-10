#pragma once

#include <fmt/format.h>
#include <string>
#include <string_view>

#include "userver/components/component_config.hpp"
#include "userver/components/component_context.hpp"
#include "userver/formats/json/value.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/server/handlers/http_handler_json_base.hpp"
#include "userver/storages/postgres/postgres_fwd.hpp"

namespace real_medium::handlers::profiles::get {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-get-profiles";

  Handler(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& component_context);

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value&,
      userver::server::request::RequestContext& request_context) const override;

 private:
  const userver::storages::postgres::ClusterPtr cluster_;
};

}  // namespace real_medium::handlers::profiles::get
