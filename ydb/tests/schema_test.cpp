#include <string>

#include <dynamic_config/variables/YDB_QUERIES_COMMAND_CONTROL.hpp>
#include <userver/formats/json.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

#include <ydb-cpp-sdk/client/types/status/status.h>

#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/impl/cast.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using YdbSchemaNoPredefinedTables = ydb::ClientFixtureBase;
using YdbNamedSchemeQuery = utest::LogCaptureFixture<ydb::ClientFixtureBase>;

NYdb::NTable::TDescribeTableResult DescribeTableNative(ydb::TestClientsBase& clients, std::string_view table_name) {
    // We cannot test our CreateTable using our DescribeTable, because they both
    // could be wrong. Let's build the set of our tested functions starting from
    // CreateTable, and use the native DescribeTable to check.
    const auto full_table_name = ydb::impl::JoinPath(clients.GetDatabasePath(), table_name);
    auto& native_client = clients.GetNativeTableClient();
    auto session = GetFutureValueSyncChecked(native_client.GetSession()).GetSession();
    return GetFutureValueSyncChecked(session.DescribeTable(ydb::impl::ToString(full_table_name)));
}

}  // namespace

UTEST_F(YdbSchemaNoPredefinedTables, CreateAndDropTable) {
    static constexpr std::string_view kTableName = "create_and_drop_table";

    // Step 0: check that the table does not exist initially.
    UASSERT_THROW(DescribeTableNative(*this, kTableName), ydb::YdbResponseError);

    // Step 1: create a table.
    auto builder =
        NYdb::NTable::TTableBuilder{}
            .AddNonNullableColumn("key", NYdb::EPrimitiveType::String)
            .AddNullableColumn("some_id", NYdb::EPrimitiveType::Uint64)
            .AddNullableColumn("some_money", NYdb::TDecimalType{/*precision=*/22, /*scale=*/9})
            .SetPrimaryKeyColumn("key");
    UASSERT_NO_THROW(GetTableClient().CreateTable(kTableName, builder.Build()));

    // Step 2: check that the table has been created as requested.
    const auto description = DescribeTableNative(*this, kTableName).GetTableDescription();
    const auto& key_columns = description.GetPrimaryKeyColumns();
    ASSERT_EQ(key_columns.size(), 1);
    EXPECT_EQ(key_columns[0], "key");
    const auto columns = description.GetColumns();
    ASSERT_EQ(columns.size(), 3);
    {
        const auto& column = columns[0];
        EXPECT_EQ(column.Name, "key");
        const NYdb::TTypeParser parser{column.Type};
        ASSERT_EQ(parser.GetKind(), NYdb::TTypeParser::ETypeKind::Primitive);
        EXPECT_EQ(parser.GetPrimitive(), NYdb::EPrimitiveType::String);
    }
    {
        const auto& column = columns[1];
        EXPECT_EQ(column.Name, "some_id");
        NYdb::TTypeParser parser{column.Type};
        ASSERT_EQ(parser.GetKind(), NYdb::TTypeParser::ETypeKind::Optional);
        parser.OpenOptional();
        ASSERT_EQ(parser.GetKind(), NYdb::TTypeParser::ETypeKind::Primitive);
        EXPECT_EQ(parser.GetPrimitive(), NYdb::EPrimitiveType::Uint64);
        parser.CloseOptional();
    }
    {
        const auto& column = columns[2];
        EXPECT_EQ(column.Name, "some_money");
        NYdb::TTypeParser parser{column.Type};
        ASSERT_EQ(parser.GetKind(), NYdb::TTypeParser::ETypeKind::Optional);
        parser.OpenOptional();
        ASSERT_EQ(parser.GetKind(), NYdb::TTypeParser::ETypeKind::Decimal);
        const auto decimal = parser.GetDecimal();
        EXPECT_EQ(decimal.Precision, 22);
        EXPECT_EQ(decimal.Scale, 9);
    }

    // Step 3: drop the table.
    UASSERT_NO_THROW(GetTableClient().DropTable(kTableName));

    // Step 4. check that the table is no more.
    UASSERT_THROW(DescribeTableNative(*this, kTableName), ydb::YdbResponseError);
}

UTEST_F(YdbNamedSchemeQuery, UsesQueryNameForCommandControl) {
    GetConfig().Extend({{::dynamic_config::YDB_QUERIES_COMMAND_CONTROL, formats::json::FromString(R"({
             "CreateNamedSchemeTable": {
                 "client-timeout-ms": 12345,
                 "get-session-timeout-ms": 23456,
                 "attempts": 2
             }
         })")}});
    const ydb::Query query{
        "CREATE TABLE named_scheme_table (id Utf8, PRIMARY KEY (id));",
        ydb::Query::Name{"CreateNamedSchemeTable"},
        ydb::Query::LogMode::kNameOnly
    };
    GetTableClient().ExecuteSchemeQuery(query);
    EXPECT_EQ(DescribeTableNative(*this, "named_scheme_table").GetTableDescription().GetColumns().size(), 1);
    const std::string drop_statement = "DROP TABLE named_scheme_table;";
    GetTableClient().ExecuteSchemeQuery(drop_statement);

    const auto logs = GetLogCapture().Filter([](const utest::LogRecord& record) {
        return record.GetTagOptional("query_name") == "CreateNamedSchemeTable" &&
               record.GetTagOptional("stopwatch_name") == "ydb_query";
    });
    ASSERT_EQ(logs.size(), 1);
    EXPECT_EQ(logs.front().GetTag("client_timeout_ms"), "12345");
    EXPECT_EQ(logs.front().GetTag("get_session_timeout_ms"), "23456");
    EXPECT_EQ(logs.front().GetTag("max_retries"), "1");
}

UTEST_F(YdbSchemaNoPredefinedTables, SchemeQueryStringLiteral) {
    GetTableClient().ExecuteSchemeQuery("CREATE TABLE literal_scheme_table (id Utf8, PRIMARY KEY (id));");
    EXPECT_EQ(DescribeTableNative(*this, "literal_scheme_table").GetTableDescription().GetColumns().size(), 1);
    GetTableClient().ExecuteSchemeQuery("DROP TABLE literal_scheme_table;");
}

USERVER_NAMESPACE_END
