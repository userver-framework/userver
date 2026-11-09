#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/storages/mysql/exceptions.hpp>
#include <userver/storages/mysql/tests/utils.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/impl/mariadb_include.hpp>
#include <storages/mysql/settings/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr std::chrono::seconds kConnectTimeout{5};

settings::AuthSettings MakeTestAuthSettings() {
    return settings::AuthSettings{
        .database = {},
        .user = "root",
        .password = decltype(settings::AuthSettings::password){std::string{}},
    };
}

settings::ConnectionSettings MakeConnectionSettings(bool use_secure_connection) {
    return settings::ConnectionSettings{
        .statements_cache_size = 1,
        .use_secure_connection = use_secure_connection,
        .use_compression = false,
        .ip_mode = settings::IpMode::kIpV4,
    };
}

impl::Connection MakeConnection(
    clients::dns::Resolver& resolver,
    const settings::ConnectionSettings& connection_settings
) {
    const settings::EndpointInfo endpoint_info{
        .host = "127.0.0.1",
        .port = GetMysqlPort(),
    };
    const auto auth_settings = MakeTestAuthSettings();

    return impl::Connection{
        resolver,
        endpoint_info,
        auth_settings,
        connection_settings,
        engine::Deadline::FromDuration(kConnectTimeout)
    };
}

}  // namespace

UTEST(SecureConnection, TlsIsEstablishedOrConnectionIsRefused) {
    clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(), {}};

    const auto insecure_settings = MakeConnectionSettings(false);

    UEXPECT_NO_THROW(MakeConnection(resolver, insecure_settings));

    const auto secure_settings = MakeConnectionSettings(true);
    try {
        auto connection = MakeConnection(resolver, secure_settings);
        EXPECT_NE(mysql_get_ssl_cipher(&connection.GetNativeHandler()), nullptr);
    } catch (const MySQLException& ex) {
        SUCCEED() << "Connection without TLS was refused: " << ex.what();
    }
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
