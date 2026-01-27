#pragma once

#include <string_view>

#include <userver/server/handlers/http_handler_json_base.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

class ClientRunner final : public server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName{"client-runner"};

    ClientRunner(const components::ComponentConfig& config, const components::ComponentContext& context);

    auto HandleRequestJsonThrow(const HttpRequest& request, const Value& /*request_json*/, RequestContext& /*context*/)
        const -> Value override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    const ::samples::api::GreeterServiceClient client_;
};

USERVER_NAMESPACE_END
