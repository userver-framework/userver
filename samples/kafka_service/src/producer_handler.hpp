#pragma once

#include <string_view>

#include <userver/kafka/producer.hpp>

#include <userver/server/handlers/http_handler_json_base.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace kafka_sample {

class ProducerHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName{"producer-handler"};

  ProducerHandler(const components::ComponentConfig& config,
                  const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override;

 private:
  kafka::Producer& producer_;
};

}  // namespace kafka_sample
