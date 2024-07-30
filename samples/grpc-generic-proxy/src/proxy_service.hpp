#pragma once

// For testing purposes only, in your services write out userver:: namespace
// instead.
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/server/generic_service_base.hpp>

namespace samples {

class ProxyService final : public ugrpc::server::GenericServiceBase::Component {
 public:
  static constexpr std::string_view kName = "proxy-service";

  ProxyService(const components::ComponentConfig& config,
               const components::ComponentContext& context);

  void Handle(Call& call) override;

 private:
  ugrpc::client::GenericClient& client_;
};

}  // namespace samples
