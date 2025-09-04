#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/ugrpc/client/component_list.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include <client/view.hpp>
#include <http_handlers/say-hello/view.hpp>
#include <middlewares/client/auth.hpp>
#include <middlewares/client/chaos.hpp>
#include <middlewares/server/auth.hpp>
#include <middlewares/server/meta_filter.hpp>
#include <service/view.hpp>

/// [gRPC middleware sample - components registration]
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .AppendComponentList(ugrpc::server::DefaultComponentList())
                                    .AppendComponentList(ugrpc::client::MinimalComponentList())
                                    .Append<ugrpc::client::ClientFactoryComponent>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<congestion_control::Component>()
                                    .Append<samples::grpc::auth::GreeterClient>()
                                    .Append<samples::grpc::auth::GreeterServiceComponent>()
                                    .Append<samples::grpc::auth::GreeterHttpHandler>()
                                    /// [gRPC middleware sample - ugrpc registration]
                                    .Append<samples::grpc::auth::server::AuthComponent>()
                                    .Append<samples::grpc::auth::server::MetaFilterComponent>()
                                    .Append<samples::grpc::auth::client::AuthComponent>()
                                    .Append<samples::grpc::auth::client::ChaosComponent>();

    return utils::DaemonMain(argc, argv, component_list);
}
/// [gRPC middleware sample - components registration]
