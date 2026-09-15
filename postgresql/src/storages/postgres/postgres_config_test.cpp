#include <userver/utest/utest.hpp>

#include <ranges>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

#include <storages/postgres/postgres_config.hpp>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/registered_config_meta.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kPostgresConfigNames[]{
    "POSTGRES_DEFAULT_COMMAND_CONTROL",
    "POSTGRES_HANDLERS_COMMAND_CONTROL",
    "POSTGRES_QUERIES_COMMAND_CONTROL",
    "POSTGRES_CONNECTION_POOL_SETTINGS",
    "POSTGRES_TOPOLOGY_SETTINGS",
    "POSTGRES_CONNECTION_SETTINGS",
    "POSTGRES_STATEMENT_METRICS_SETTINGS",
};

UTEST(PostgresConfig, CompositeConfigMetadataHasSchemaHashes) {
    EXPECT_NO_THROW(dynamic_config::GetDefaultSnapshot()[storages::postgres::kConfig]);

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    for (const auto expected_name : kPostgresConfigNames) {
        SCOPED_TRACE(expected_name);
        auto matching_configs =
            registered_configs |
            std::views::filter([expected_name](const auto& metadata) { return metadata.name == expected_name; });
        const auto metadata = utils::AsContainer<std::vector<dynamic_config::RegisteredConfigMeta>>(matching_configs);
        EXPECT_THAT(metadata, testing::Not(testing::IsEmpty()));
        EXPECT_THAT(
            metadata,
            testing::Each(testing::Field(
                "schema_hash",
                &dynamic_config::RegisteredConfigMeta::schema_hash,
                testing::Not(testing::IsEmpty())
            ))
        );
    }
}

}  // namespace

USERVER_NAMESPACE_END
