#include <client_runner.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

ClientRunner::ClientRunner(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase{config, context},
      client_{context.FindComponent<ugrpc::client::ClientFactoryComponent>()
                  .GetFactory()
                  .MakeClient<::samples::api::GreeterServiceClient>(
                      "client",
                      config["server-endpoint"].As<std::string>()
                  )} {}

auto ClientRunner::HandleRequestJsonThrow(
    const HttpRequest& /*request*/,
    const Value& /*request_json*/,
    RequestContext& /*context*/
) const -> Value {
    std::optional<::grpc::Status> grpc_status;
    try {
        [[maybe_unused]] auto response = client_.SayHello({});
        grpc_status.emplace(::grpc::Status::OK);
    } catch (const ugrpc::client::ErrorWithStatus& ex) {
        grpc_status.emplace(ex.GetStatus());
    } catch (const std::exception& ex) {
        return {};
    }

    return formats::json::MakeObject("grpc-status", ugrpc::ToString(grpc_status->error_code()));
}

yaml_config::Schema ClientRunner::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<server::handlers::HttpHandlerJsonBase>(R"(
type: object
description: gRPC client runner
additionalProperties: false
properties:
    server-endpoint:
        description: endpoint http2 server is listening
        type: string
)");
}

USERVER_NAMESPACE_END
