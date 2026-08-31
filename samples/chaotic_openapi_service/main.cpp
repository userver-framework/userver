#include <userver/chaotic/openapi/middlewares/component_list.hpp>
#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>
#include <userver/chaotic/openapi/server/dependencies.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/digest/nonce_cache_settings_component.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <clients/secure/component.hpp>
#include <clients/test/component.hpp>
#include <clients/test/qos.hpp>

#include <auth_digest.hpp>
#include <handlers/insecure/chaotic_handlers_list.hpp>
#include <handlers/secure/chaotic_handlers_list.hpp>
#include <hello_handler.hpp>

#include "auth_bearer.hpp"

int main(int argc, char* argv[]) {
    /// [digest-auth-checker-registration]
    USERVER_NAMESPACE::server::handlers::auth::RegisterAuthCheckerFactory<
        samples::chaotic_openapi_service::CheckerFactory>("myDigestScheme");
    /// [digest-auth-checker-registration]

    auto component_list =
        USERVER_NAMESPACE::components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .Append<USERVER_NAMESPACE::components::Secdist>()
            .Append<USERVER_NAMESPACE::components::DefaultSecdistProvider>()
            /// [digest-settings-component]
            .Append<USERVER_NAMESPACE::server::handlers::auth::digest::NonceCacheSettingsComponent>()
            /// [digest-settings-component]
            .Append<samples::hello::HelloHandler>()
            .Append<USERVER_NAMESPACE::components::TestsuiteSupport>()
            .Append<USERVER_NAMESPACE::server::handlers::TestsControl>()
            .AppendComponentList(USERVER_NAMESPACE::clients::http::ComponentList())
            .Append<USERVER_NAMESPACE::clients::dns::Component>()
            /// [register-qos]
            .Append<USERVER_NAMESPACE::chaotic::openapi::QosMiddlewareFactory<
                ::clients::test::kQosConfig>>("chaotic-client-middleware-qos-test")
            /// [register-qos]
            /// [register-client]
            .Append<::clients::test::Component>()
            /// [register-client]
            /// [register-digest-client]
            .Append<::clients::secure::Component>()
            /// [register-digest-client]
            /// [register-handlers]
            .Append<USERVER_NAMESPACE::components::Container<
                USERVER_NAMESPACE::chaotic::openapi::server::dependencies::Factories>>()
            .AppendComponentList(::handlers::insecure::ChaoticHandlersList())
            .AppendComponentList(::handlers::secure::ChaoticHandlersList());
    /// [register-handlers]

    USERVER_NAMESPACE::chaotic::openapi::middlewares::AppendDefaultMiddlewares(component_list);

    server::handlers::auth::RegisterAuthCheckerFactory<samples::auth::CheckerFactory>();

    return USERVER_NAMESPACE::utils::DaemonMain(argc, argv, component_list);
}
