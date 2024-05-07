#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ydb/component.hpp>

namespace sample {

class BaseHandler : public server::handlers::HttpHandlerJsonBase {
 public:
  BaseHandler(const components::ComponentConfig& config,
              const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        ydb_client_(context.FindComponent<ydb::YdbComponent>().GetTableClient(
            "sampledb")) {}

 protected:
  ydb::TableClient& Ydb() const { return *ydb_client_; }

 private:
  std::shared_ptr<ydb::TableClient> ydb_client_;
};

}  // namespace sample
