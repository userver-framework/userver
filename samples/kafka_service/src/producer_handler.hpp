#pragma once

#include <string_view>

/// [Kafka service sample - producer include]

#include <userver/kafka/producer.hpp>

/// [Kafka service sample - producer include]

#include <userver/server/handlers/http_handler_json_base.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace kafka_sample {

/// [Kafka service sample - producer component declaration]
class ProducerHandler final : public server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName{"producer-handler"};

    ProducerHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

    formats::json::Value HandleRequestJsonThrow(
        const server::http::HttpRequest& request,
        const formats::json::Value& request_json,
        server::request::RequestContext& context
    ) const override;

private:
    const kafka::Producer& producer_;
};
/// [Kafka service sample - producer component declaration]

}  // namespace kafka_sample
