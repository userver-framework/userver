#include "auth_bearer.hpp"

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <handlers/secure/chaotic_handlers_list.hpp>

int main(int argc, char* argv[]) {
    /// [auth checker registration]
    server::handlers::auth::RegisterAuthCheckerFactory<samples::auth::CheckerFactory>();
    /// [auth checker registration]

    auto component_list = components::MinimalServerComponentList()
        .Append<components::TestsuiteSupport>()
        /// [register-secure-handlers]
        .AppendComponentList(::handlers::secure::ChaoticHandlersList());
    /// [register-secure-handlers]

    return utils::DaemonMain(argc, argv, component_list);
}