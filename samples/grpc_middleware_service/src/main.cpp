#include "client/view.hpp"
#include "http_handlers/say-hello/view.hpp"
#include "middlewares/client/component.hpp"
#include "middlewares/server/component.hpp"
#include "service/view.hpp"

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

/// [gRPC middleware sample - components registration]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<samples::grpc::auth::GreeterClient>()
          .Append<samples::grpc::auth::GreeterServiceComponent>()
          .Append<samples::grpc::auth::GreeterHttpHandler>()
          /// [gRPC middleware sample - ugrpc registration]
          .Append<sample::grpc::auth::client::Component>()
          .Append<sample::grpc::auth::server::Component>();

  return utils::DaemonMain(argc, argv, component_list);
}
/// [gRPC middleware sample - components registration]
