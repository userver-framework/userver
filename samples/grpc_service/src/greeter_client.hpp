#pragma once

#include <string_view>

#include <userver/utest/using_namespace_userver.hpp>

/// [includes]
#include <userver/components/component_base.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/ugrpc/client/fwd.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
/// [includes]

namespace samples {

/// [client]
// A user-defined wrapper around api::GreeterServiceClient that handles
// the metadata and deadline bureaucracy and provides a simplified interface.
//
// Alternatively, you can use ugrpc::client::SimpleClientComponent directly.
//
// Note that we have both service and client to that service in the same
// microservice. Ignore that, it's just for the sake of example.
class GreeterClient final {
 public:
  explicit GreeterClient(api::GreeterServiceClient&& raw_client);

  std::string SayHello(std::string name) const;

 private:
  api::GreeterServiceClient raw_client_;
};
/// [client]

/// [component]
class GreeterClientComponent final : public components::ComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-client";

  GreeterClientComponent(const components::ComponentConfig& config,
                         const components::ComponentContext& context);

  const GreeterClient& GetClient() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  ugrpc::client::ClientFactory& client_factory_;
  GreeterClient client_;
};
/// [component]

}  // namespace samples
