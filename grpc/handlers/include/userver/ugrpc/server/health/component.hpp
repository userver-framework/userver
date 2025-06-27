#pragma once

#include <userver/ugrpc/server/service_component_base.hpp>
#include <userver/utils/box.hpp>

namespace grpc::health::v1 {
class HealthHandler;
}  // namespace grpc::health::v1

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class HealthComponent final : public ServiceComponentBase {
public:
    static constexpr std::string_view kName = "grpc-health";

    HealthComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~HealthComponent() override;

private:
    utils::Box<::grpc::health::v1::HealthHandler> service_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
