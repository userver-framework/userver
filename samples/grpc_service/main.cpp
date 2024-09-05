#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/common_component.hpp>
#include <userver/ugrpc/server/server_component.hpp>

#include <call_greeter_client_test_handler.hpp>
#include <greeter_client.hpp>
#include <greeter_service.hpp>

/// [main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          // Contains machinery common to all gRPC clients
          .Append<ugrpc::client::CommonComponent>()
          // Default client factory. You can create multiple instances of this
          // component using `.Append<T>("name")` if different gRPC clients
          // require different credentials or different grpc-core options.
          .Append<ugrpc::client::ClientFactoryComponent>()
          // All gRPC services are registered in this component.
          .Append<ugrpc::server::ServerComponent>()
          // Custom components:
          .Append<samples::GreeterClientComponent>()
          .Append<samples::GreeterServiceComponent>()
          .Append<samples::CallGreeterClientTestHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [main]
