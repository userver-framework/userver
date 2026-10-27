#include <string_view>

#include <gtest/gtest.h>

#include <userver/formats/json.hpp>

#include <storages/clickhouse/impl/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

storages::clickhouse::impl::ClickhouseSettingsMulti ParseSettings(const std::string_view secret_json) {
    formats::json::ValueBuilder secret{formats::json::FromString(secret_json)};
    secret["hosts"] = formats::json::MakeArray("localhost", "replica");
    secret["user"] = "reader";
    secret["password"] = "secret";
    formats::json::ValueBuilder doc;
    doc["clickhouse_settings"]["analytics"] = secret.ExtractValue();
    return storages::clickhouse::impl::ClickhouseSettingsMulti{doc.ExtractValue()};
}

}  // namespace

TEST(ClickhouseSettings, ExplicitPortAndDatabase) {
    const auto settings = ParseSettings(R"({"port": 9000, "dbname": "reports"})").Get("analytics");

    EXPECT_EQ(settings.auth_settings.database, "reports");
    EXPECT_EQ(settings.auth_settings.user, "reader");
    EXPECT_EQ(settings.auth_settings.password, "secret");
    ASSERT_EQ(settings.endpoints.size(), 2);
    EXPECT_EQ(settings.endpoints[0].host, "localhost");
    EXPECT_EQ(settings.endpoints[1].host, "replica");
    for (const auto& endpoint : settings.endpoints) {
        EXPECT_EQ(endpoint.port, 9000);
    }
}

TEST(ClickhouseSettings, NativePortAndDefaultDatabase) {
    const auto settings = ParseSettings(R"({"port_client": 9440, "port_http": 8443})").Get("analytics");

    EXPECT_EQ(settings.auth_settings.database, "default");
    ASSERT_EQ(settings.endpoints.size(), 2);
    for (const auto& endpoint : settings.endpoints) {
        EXPECT_EQ(endpoint.port, 9440);
    }
}

TEST(ClickhouseSettings, PortTakesPrecedenceOverPortClient) {
    const auto settings = ParseSettings(R"({"port": 9000, "port_client": 9440})").Get("analytics");

    ASSERT_EQ(settings.endpoints.size(), 2);
    for (const auto& endpoint : settings.endpoints) {
        EXPECT_EQ(endpoint.port, 9000);
    }
}

TEST(ClickhouseSettings, DatabaseOverrideWithoutSecretDatabase) {
    const auto settings = ParseSettings(R"({"port": 9000})");

    EXPECT_EQ(settings.Get("analytics", "reports").auth_settings.database, "reports");
    EXPECT_EQ(settings.Get("analytics", "metrics").auth_settings.database, "metrics");
    EXPECT_EQ(settings.Get("analytics").auth_settings.database, "default");
}

TEST(ClickhouseSettings, ComponentDatabaseOverridesSecretWithoutMutatingIt) {
    const auto settings = ParseSettings(R"({"port": 9000, "dbname": "reports"})");

    EXPECT_EQ(settings.Get("analytics", "metrics").auth_settings.database, "metrics");
    EXPECT_EQ(settings.Get("analytics", "default").auth_settings.database, "default");
    EXPECT_EQ(settings.Get("analytics").auth_settings.database, "reports");
}

USERVER_NAMESPACE_END
