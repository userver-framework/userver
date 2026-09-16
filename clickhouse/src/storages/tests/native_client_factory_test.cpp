#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/yaml.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <storages/clickhouse/impl/native_client_factory.hpp>
#include <storages/clickhouse/impl/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

storages::clickhouse::impl::ConnectionSettings MakeNonSecureConnectionSettings() {
    return yaml_config::YamlConfig{formats::yaml::FromString("use_secure_connection: false"), {}}
        .As<storages::clickhouse::impl::ConnectionSettings>();
}

}  // namespace

UTEST(NativeClientFactory, ConnectFailureIncludesErrors) {
    clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};
    const auto connection_settings = MakeNonSecureConnectionSettings();

    storages::clickhouse::impl::AuthSettings auth;
    auth.user = "default";
    auth.password = "";
    auth.database = "default";

    const storages::clickhouse::impl::EndpointSettings endpoint{
        .host = "127.0.0.1",
        .port = 1,
    };

    try {
        storages::clickhouse::impl::NativeClientFactory::Create(resolver, endpoint, auth, connection_settings);
        FAIL() << "expected connection failure";
    } catch (const std::exception& e) {
        EXPECT_THAT(
            e.what(),
            testing::AllOf(
                testing::HasSubstr("Could not connect to any of the resolved addresses"),
                testing::HasSubstr("Errors:"),
                testing::HasSubstr("127.0.0.1")
            )
        );
    }
}

USERVER_NAMESPACE_END
