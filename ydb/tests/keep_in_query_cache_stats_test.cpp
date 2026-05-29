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

constexpr std::string_view kYamlDatabaseNoCache = R"(
keep-in-query-cache: false
use-query-client: false
)";

constexpr std::string_view kYamlDatabaseWithCache = R"(
keep-in-query-cache: true
use-query-client: false
)";

ydb::impl::TableSettings TableSettingsFromDatabaseYaml(std::string_view yaml) {
    const yaml_config::YamlConfig config{formats::yaml::FromString(std::string{yaml}), {}};
    return ydb::impl::ParseTableSettings(config, ydb::impl::secdist::DatabaseSettings{});
}

class YdbTableDatabaseYamlNoCacheFixture : public YdbSmallTableTest {
protected:
    ydb::impl::TableSettings GetTableSettings() const override {
        return TableSettingsFromDatabaseYaml(kYamlDatabaseNoCache);
    }
};

class YdbTableDatabaseYamlWithCacheFixture : public YdbSmallTableTest {
protected:
    ydb::impl::TableSettings GetTableSettings() const override {
        return TableSettingsFromDatabaseYaml(kYamlDatabaseWithCache);
    }
};

const ydb::Query kSelectByKeyQuery{R"(
    DECLARE $search_key AS String;

    SELECT key, value_str
    FROM test_table
    WHERE key = $search_key;
)"};

UTEST_F(YdbTableDatabaseYamlNoCacheFixture, TableServiceWithoutKeepInQueryCache) {
    const auto table_settings = TableSettingsFromDatabaseYaml(kYamlDatabaseNoCache);
    EXPECT_FALSE(table_settings.keep_in_query_cache);
    EXPECT_FALSE(table_settings.use_query_client);

    CreateTable("test_table", true);

    ydb::QuerySettings query_settings;
    query_settings.collect_query_stats = NYdb::NTable::ECollectQueryStatsMode::Basic;

    auto& client = GetTableClient();
    for (std::uint64_t i = 0; i < 2; ++i) {
        const ydb::Query query{fmt::format(
            R"(
DECLARE $search_key AS String;

SELECT key, value_str
FROM test_table
WHERE key = $search_key AND CAST({0} AS Uint8) = CAST({0} AS Uint8);
)",
            i
        )};

        auto builder = client.GetBuilder();
        builder.Add("$search_key", std::string{"key1"});

        auto response = client.ExecuteDataQuery(query_settings, ydb::OperationSettings{}, query, std::move(builder));

        ASSERT_TRUE(response.GetQueryStats().has_value());
        EXPECT_FALSE(response.IsFromServerQueryCache()) << "iteration " << i;
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

UTEST_F(YdbTableDatabaseYamlWithCacheFixture, TableServiceWithKeepInQueryCache) {
    const auto table_settings = TableSettingsFromDatabaseYaml(kYamlDatabaseWithCache);
    EXPECT_TRUE(table_settings.keep_in_query_cache);
    EXPECT_FALSE(table_settings.use_query_client);

    CreateTable("test_table", true);

    ydb::QuerySettings query_settings;
    query_settings.collect_query_stats = NYdb::NTable::ECollectQueryStatsMode::Basic;

    auto& client = GetTableClient();
    for (std::uint64_t i = 0; i < 2; ++i) {
        auto builder = client.GetBuilder();
        builder.Add("$search_key", std::string{"key1"});

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
