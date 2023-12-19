#pragma once

#include <userver/ugrpc/server/service_component_base.hpp>

#include <userver/ugrpc/server/health/health.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class HealthComponent final : public ugrpc::server::ServiceComponentBase {
 public:
  static constexpr std::string_view kName = "grpc-health";

  HealthComponent(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
      : ugrpc::server::ServiceComponentBase(config, context),
        service_(context) {
    RegisterService(service_);
  }

 private:
  HealthHandler service_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
