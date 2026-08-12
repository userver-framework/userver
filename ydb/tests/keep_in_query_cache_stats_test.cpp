#include <fmt/format.h>

#include <ydb-cpp-sdk/client/query/query.h>

#include <userver/formats/yaml.hpp>
#include <userver/utest/utest.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/secdist.hpp>

#include "small_table.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kYamlDatabaseDeprecatedOptions = R"(
keep-in-query-cache: false
use-query-client: false
)";

ydb::impl::TableSettings TableSettingsFromDatabaseYaml(std::string_view yaml) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(std::string{yaml}), {}};
    return ydb::impl::ParseTableSettings(config, ydb::impl::secdist::DatabaseSettings{});
}

const ydb::Query kSelectByKeyQuery{R"(
    DECLARE $search_key AS String;

    SELECT key, value_str
    FROM test_table
    WHERE key = $search_key;
)"};

UTEST_F(YdbSmallTableTest, DeprecatedConfigKeysAreParsedButIgnored) {
    const auto table_settings = TableSettingsFromDatabaseYaml(kYamlDatabaseDeprecatedOptions);
    EXPECT_FALSE(table_settings.keep_in_query_cache);
    EXPECT_FALSE(table_settings.use_query_client);
}

UTEST_F(YdbSmallTableTest, ExecuteDataQueryUsesQueryApiCache) {
    CreateTable("test_table", true);

    auto& client = GetTableClient();
    for (std::uint64_t i = 0; i < 2; ++i) {
        auto builder = client.GetBuilder();
        builder.Add("$search_key", std::string{"key1"});

        ydb::QuerySettings query_settings;
        query_settings.collect_query_stats = NYdb::NTable::ECollectQueryStatsMode::Basic;

        auto response =
            client.ExecuteDataQuery(query_settings, ydb::OperationSettings{}, kSelectByKeyQuery, std::move(builder));

        ASSERT_TRUE(response.GetQueryStats().has_value());
        EXPECT_TRUE(i == 0 || response.IsFromServerQueryCache());
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

UTEST_F(YdbSmallTableTest, QueryServiceExecuteQuery) {
    CreateTable("test_table", true);

    auto& client = GetTableClient();
    for (std::uint64_t i = 0; i < 2; ++i) {
        auto builder = client.GetBuilder();
        builder.Add("$search_key", std::string{"key1"});

        NYdb::NQuery::TExecuteQuerySettings exec_settings;
        exec_settings.StatsMode(NYdb::NQuery::EStatsMode::Basic);

        auto response = client.ExecuteQuery(
            std::move(exec_settings),
            ydb::OperationSettings{},
            kSelectByKeyQuery,
            std::move(builder)
        );

        ASSERT_TRUE(response.GetQueryStats().has_value());
        EXPECT_TRUE(i == 0 || response.IsFromServerQueryCache());
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

}  // namespace

USERVER_NAMESPACE_END
