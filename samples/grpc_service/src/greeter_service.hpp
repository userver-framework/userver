#pragma once

#include <string_view>
#include <userver/utest/using_namespace_userver.hpp>

/// [includes]
#include <userver/components/component_fwd.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>
/// [includes]

namespace samples {

/// [service]
class GreeterService final : public api::GreeterServiceBase {
public:
    explicit GreeterService(std::string prefix);

    SayHelloResult SayHello(CallContext& context, api::GreetingRequest&& request) override;

    SayHelloResponseStreamResult SayHelloResponseStream(
        CallContext& context,
        api::GreetingRequest&& request,
        SayHelloResponseStreamWriter& writer
    ) override;

    SayHelloRequestStreamResult SayHelloRequestStream(CallContext& context, SayHelloRequestStreamReader& reader)
        override;

    SayHelloStreamsResult SayHelloStreams(CallContext& context, SayHelloStreamsReaderWriter& stream) override;

private:
    const std::string prefix_;
};
/// [service]

/// [component]
class GreeterServiceComponent final : public ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "greeter-service";

    GreeterServiceComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    GreeterService service_;
};
/// [component]

}  // namespace samples
