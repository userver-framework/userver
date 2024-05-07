#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/ydb/component.hpp>

namespace sample {

class BsonUpsertingHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-bson-upserting";

  BsonUpsertingHandler(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        ydb_client_(context.FindComponent<ydb::YdbComponent>().GetTableClient(
            "sampledb")) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

 private:
  std::shared_ptr<ydb::TableClient> ydb_client_;
};

}  // namespace sample
