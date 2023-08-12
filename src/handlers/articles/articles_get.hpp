#pragma once

#include <fmt/format.h>
#include <string_view>

#include "userver/formats/json/value.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/server/handlers/http_handler_json_base.hpp"

#include "userver/storages/postgres/cluster.hpp"
#include "userver/storages/postgres/component.hpp"

#include "userver/components/component_config.hpp"
#include "userver/components/component_context.hpp"

namespace real_medium::handlers::articles::get {

class Handler final : public userver::server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-get-articles";
  Handler(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& component_context);
  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest& request,
      const userver::formats::json::Value& request_json,
      userver::server::request::RequestContext& context) const override final;
  using HttpHandlerJsonBase::HttpHandlerJsonBase;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace real_medium::handlers::articles::get
