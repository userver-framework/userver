#pragma once

#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/statistics/metrics_storage_fwd.hpp>

namespace tests::handlers {

class Metrics final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-metrics";

  Metrics(const components::ComponentConfig&,
          const components::ComponentContext&);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_body,
      server::request::RequestContext& context) const override;

 private:
  utils::statistics::MetricsStoragePtr metrics_;
};

}  // namespace tests::handlers
