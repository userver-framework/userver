// grpc template on
#include <hello_grpc.hpp>

#include <greeting.hpp>

namespace service_template {

HelloGrpc::HelloGrpc(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : handlers::api::HelloServiceBase::Component(config, component_context) {}

HelloGrpc::SayHelloResult HelloGrpc::SayHello(CallContext&, handlers::api::HelloRequest&& request) {
    handlers::api::HelloResponse response;
    response.set_text(SayHelloTo(request.name(), UserType::kFirstTime));
    return response;
}

}  // namespace service_template
// grpc template off
