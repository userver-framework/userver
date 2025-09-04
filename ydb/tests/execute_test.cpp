#include <chrono>
#include <thread>

#include <userver/utest/utest.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/impl/cast.hpp>

#include "small_table.hpp"
#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
class YdbExecute : public YdbSmallTableTest {};

template <typename ExecuteImpl>
class YdbExecuteTpl : public YdbSmallTableTest {
public:
    template <typename... Args>
    auto Execute(Args&&... args) {
        return ExecuteImpl::DoExecute(GetTableClient(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto ExecuteWithBasicStats(Args&&... args) {
        return ExecuteImpl::DoExecuteWithBasicStats(GetTableClient(), std::forward<Args>(args)...);
    }
};

struct ExecuteDataQuery {
    template <typename... Args>
    static auto DoExecute(ydb::TableClient& client, Args&&... args) {
        return client.ExecuteDataQuery(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static auto DoExecuteWithBasicStats(ydb::TableClient& client, Args&&... args) {
        ydb::QuerySettings query_settings;
        query_settings.collect_query_stats = NYdb::NTable::ECollectQueryStatsMode::Basic;

        return client.ExecuteDataQuery(std::move(query_settings), std::forward<Args>(args)...);
    }
};

struct ExecuteQuery {
    template <typename... Args>
    static auto DoExecute(ydb::TableClient& client, Args&&... args) {
        return client.ExecuteQuery(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static auto DoExecuteWithBasicStats(ydb::TableClient& client, Args&&... args) {
        NYdb::NQuery::TExecuteQuerySettings exec_settings;
        exec_settings.StatsMode(NYdb::NQuery::EStatsMode::Basic);

        return client.ExecuteQuery(std::move(exec_settings), std::forward<Args>(args)...);
    }
};
}  // namespace

UTEST_F(YdbExecute, ExecuteDataQuery) {
    CreateTable("simple_write", false);

    ydb::OperationSettings settings;
    settings.tx_mode = ydb::TransactionMode::kSerializableRW;
    const ydb::Query upsert_query{R"(
    UPSERT INTO simple_write (key, value_str, value_int)
    VALUES ("key1", "value1", 1), ("key2", "value2", 2);
  )"};
    const auto upsert_response = GetTableClient().ExecuteDataQuery(settings, upsert_query);
    ASSERT_EQ(upsert_response.GetCursorCount(), 0);

    // We need to check our ExecuteDataQuery using native read operations,
    // otherwise we could miss a potential bug in both our write and read ops.
    auto session = GetFutureValueSyncChecked(GetNativeTableClient().GetSession()).GetSession();
    auto tx = NYdb::NTable::TTxControl::BeginTx().CommitTx();
    const std::string_view select_query = R"(
    SELECT key, value_str, value_int
    FROM simple_write
    ORDER BY key;
  )";
    auto select_response = GetFutureValueSyncChecked(session.ExecuteDataQuery(ydb::impl::ToString(select_query), tx));
    ASSERT_EQ(select_response.GetResultSets().size(), 1);
    auto result_parser = select_response.GetResultSetParser(0);
    ASSERT_TRUE(result_parser.TryNextRow());
    EXPECT_EQ(result_parser.ColumnParser("key").GetOptionalString(), "key1");
    EXPECT_EQ(result_parser.ColumnParser("value_str").GetOptionalString(), "value1");
    EXPECT_EQ(result_parser.ColumnParser("value_int").GetOptionalInt32(), 1);
    ASSERT_TRUE(result_parser.TryNextRow());
    EXPECT_EQ(result_parser.ColumnParser("key").GetOptionalString(), "key2");
    EXPECT_EQ(result_parser.ColumnParser("value_str").GetOptionalString(), "value2");
    EXPECT_EQ(result_parser.ColumnParser("value_int").GetOptionalInt32(), 2);
    ASSERT_FALSE(result_parser.TryNextRow());
}

using MyTypes = testing::Types<ExecuteDataQuery, ExecuteQuery>;
TYPED_UTEST_SUITE(YdbExecuteTpl, MyTypes);

TYPED_UTEST(YdbExecuteTpl, SimpleRead) {
    this->CreateTable("simple_read", true);

    auto response = this->Execute(ydb::Query{R"(
        SELECT key, value_str
        FROM simple_read
        WHERE key IN ("key1", "key3")
        ORDER BY key DESC;
    )"});
    auto cursor = response.GetSingleCursor();
    ASSERT_FALSE(cursor.IsTruncated());
    AssertArePreFilledRows(std::move(cursor), {3, 1});
}

TYPED_UTEST(YdbExecuteTpl, SimpleStaleRead) {
    this->CreateTable("stale_read", true);

    const int retries_count = 10;
    for (int i = 1; i <= retries_count; i++) {
        ydb::OperationSettings settings;
        settings.tx_mode = ydb::TransactionMode::kStaleRO;
        auto response = this->Execute(settings, ydb::Query{R"(
            --!syntax_v1
            SELECT key
            FROM `stale_read` view value_idx
            WHERE value_int = 3;
        )"});
        auto cursor = response.GetSingleCursor();

        if (!cursor.empty() || (i == retries_count)) {
            for (auto row : cursor) {
                AssertNullableColumn(row, "key", "key3");
            }
            ASSERT_EQ(cursor.size(), 1);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

TYPED_UTEST(YdbExecuteTpl, SimpleMultiCursorRead) {
    this->CreateTable("simple_read", true);
    auto response = this->Execute(ydb::Query{R"(
        SELECT key FROM simple_read WHERE value_int=1;
        SELECT key FROM simple_read WHERE value_int=2;
        SELECT key FROM simple_read WHERE value_int=3;
    )"});
    ASSERT_EQ(response.GetCursorCount(), 3);

    for (int i = 0; i < 3; i++) {
        auto cursor = response.GetCursor(i);
        ASSERT_EQ(cursor.size(), 1) << "One row in cursor";
        bool was_updated = false;
        for (auto row : cursor) {
            was_updated = true;
            AssertNullableColumn(row, "key", kPreFilledRows[i].key);
        }
        ASSERT_TRUE(was_updated) << "Expected the non-empty cursor";
    }
}

TYPED_UTEST(YdbExecuteTpl, PreparedRead) {
    this->CreateTable("prepared_read", true);

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, value_str
        FROM prepared_read
        WHERE key = $search_key;
    )"};

    {
        auto builder = this->GetTableClient().GetBuilder();
        UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key1"}));

        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder));
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
    {
        auto builder = this->GetTableClient().GetBuilder();
        UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key3"}));
        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder));
        AssertArePreFilledRows(response.GetSingleCursor(), {3});
    }
}

TYPED_UTEST(YdbExecuteTpl, PreparedReadSafe) {
    this->CreateTable("prepared_read_safe", true);

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, value_str
        FROM prepared_read_safe
        WHERE key = $search_key;
    )"};

    {
        auto builder = this->GetTableClient().GetBuilder();
        UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key1"}));
        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder));
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
    {
        auto builder = this->GetTableClient().GetBuilder();
        UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key3"}));
        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder));
        AssertArePreFilledRows(response.GetSingleCursor(), {3});
    }
}

TYPED_UTEST(YdbExecuteTpl, PreparedVectorRead) {
    this->CreateTable("prepared_vector_read", true);

    const ydb::Query query{R"(
        DECLARE $search_keys AS List<String>;

        SELECT key, value_str
        FROM prepared_vector_read
        WHERE key IN $search_keys
        ORDER BY key;
    )"};

    auto builder = this->GetTableClient().GetBuilder();
    std::vector<std::string> rows{"key1", "key3"};
    UASSERT_NO_THROW(builder.Add("$search_keys", rows));

    auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder));
    AssertArePreFilledRows(response.GetSingleCursor(), {1, 3});
}

TYPED_UTEST(YdbExecuteTpl, CursorMoveDoesNotInvalidateRow) {
    this->CreateTable("test_table", true);

    auto response = this->Execute(kSelectAllRows);
    auto cursor = response.GetSingleCursor();

    auto row = cursor.GetFirstRow();
    auto cursor_moved = std::move(cursor);
    AssertIsPreFilledRow(std::move(row), 1);
}

TYPED_UTEST(YdbExecuteTpl, SimpleWrite) {
    this->CreateTable("simple_write_async", false);

    auto response = this->Execute(ydb::Query{R"(
      UPSERT INTO simple_write_async (key, value_str, value_int)
      VALUES ("key1", "value1", 1), ("key2", "value2", 2);
  )"});

    ASSERT_EQ(response.GetCursorCount(), 0);
}

TYPED_UTEST(YdbExecuteTpl, InsertRowWrite) {
    this->CreateTable("prepared_vector_struct_write", false);

    const ydb::Query query{R"(
        --!syntax_v1

        DECLARE $items AS List<Struct<'key': String, 'value_str': String, 'value_int': Int32>>;

        UPSERT INTO prepared_vector_struct_write
        SELECT * FROM AS_TABLE($items);
    )"};

    auto row = ydb::InsertRow{
        ydb::InsertColumn{"key", std::string{"key1"}},
        ydb::InsertColumn{"value_str", std::string{"value1"}},
        ydb::InsertColumn{"value_int", int32_t{1}},
    };
    const std::vector<ydb::InsertRow> rows{row};

    auto response = this->Execute(ydb::OperationSettings{}, query, "$items", rows);
    ASSERT_EQ(response.GetCursorCount(), 0);
}

TYPED_UTEST(YdbExecuteTpl, PreparedReadIndependentParams) {
    this->CreateTable("prepared_read_params", true);

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, value_str
        FROM prepared_read_params
        WHERE key = $search_key;
    )"};

    // Create first params
    auto builder1 = this->GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder1.Add("$search_key", std::string{"key1"}));

    // Create second params
    auto builder2 = this->GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder2.Add("$search_key", std::string{"key3"}));

    // Execute second params
    {
        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder2));
        AssertArePreFilledRows(response.GetSingleCursor(), {3});
    }

    // Execute first params
    {
        auto response = this->Execute(ydb::OperationSettings{}, query, std::move(builder1));
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

UTEST_F(YdbExecute, SimpleDirectoryWorkflow) {
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("new-dir"));

    auto info = GetTableClient().DescribePath("new-dir");
    const auto& entry = info.GetEntry();

    ASSERT_EQ(entry.Name, "new-dir");
    ASSERT_EQ(entry.Type, NYdb::NScheme::ESchemeEntryType::Directory);

    UASSERT_NO_THROW(GetTableClient().RemoveDirectory("new-dir"));

    UASSERT_THROW(GetTableClient().DescribePath("new-dir"), ydb::YdbResponseError);
}

UTEST_F(YdbExecute, DescribeDifferentTypes) {
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("types"));

    CreateTable("types/table", false);
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("types/dir"));

    auto table_info = GetTableClient().DescribePath("types/table");
    auto dir_info = GetTableClient().DescribePath("types/dir");

    ASSERT_EQ(table_info.GetEntry().Type, NYdb::NScheme::ESchemeEntryType::Table);
    ASSERT_EQ(dir_info.GetEntry().Type, NYdb::NScheme::ESchemeEntryType::Directory);
}

UTEST_F(YdbExecute, ListDirectory) {
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("list"));

    for (char c = 'a'; c < 'z'; ++c) {
        auto inner_dir = std::string("list/") + c;
        UASSERT_NO_THROW(GetTableClient().MakeDirectory(inner_dir));
    }

    auto info = GetTableClient().ListDirectory("list");

    ASSERT_EQ(info.GetChildren().size(), 'z' - 'a');

    for (const auto& entry : info.GetChildren()) {
        ASSERT_EQ(entry.Type, NYdb::NScheme::ESchemeEntryType::Directory);
        ASSERT_EQ(entry.Name.size(), 1);
    }
}

UTEST_F(YdbExecute, Consistency) {
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("consistency"));

    CreateTable("consistency/table", false);
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("consistency/dir-1"));
    UASSERT_NO_THROW(GetTableClient().MakeDirectory("consistency/dir-2"));

    auto info = GetTableClient().ListDirectory("consistency");
    ASSERT_EQ(info.GetChildren().size(), 3);

    for (const auto& entry : info.GetChildren()) {
        auto entry_path = std::string("consistency/") + entry.Name;
        auto describe_info = GetTableClient().DescribePath(entry_path);
        ASSERT_EQ(entry.Name, describe_info.GetEntry().Name);
        ASSERT_EQ(entry.Owner, describe_info.GetEntry().Owner);
        ASSERT_EQ(entry.Type, describe_info.GetEntry().Type);
        ASSERT_EQ(entry.SizeBytes, describe_info.GetEntry().SizeBytes);
        ASSERT_EQ(entry.CreatedAt, describe_info.GetEntry().CreatedAt);
    }
}

TYPED_UTEST(YdbExecuteTpl, BulkUpsert) {
    constexpr auto kBulkSize = 10;
    this->CreateTable("bulk_upsert", false);

    NYdb::TValueBuilder rows;
    rows.BeginList();
    for (auto i = 0; i < kBulkSize; ++i) {
        const auto suffix = std::to_string(i);
        rows.AddListItem()
            .BeginStruct()
            .AddMember("key")
            .String("key" + suffix)
            .AddMember("value_str")
            .String("value" + suffix)
            .AddMember("value_int")
            .Int32(i)
            .EndStruct();
    }
    rows.EndList();

    this->GetTableClient().BulkUpsert("bulk_upsert", rows.Build());

    auto result = this->Execute(ydb::Query{"SELECT key, value_str, value_int FROM bulk_upsert"});
    ASSERT_EQ(result.GetCursorCount(), 1);
    auto cursor = result.GetSingleCursor();
    EXPECT_EQ(cursor.RowsCount(), kBulkSize);
}

UTEST_F(YdbExecute, ReadTable) {
    CreateTable("read_table", true);

    auto settings = NYdb::NTable::TReadTableSettings{}
                        .Ordered()
                        .AppendColumns("key")
                        .AppendColumns("value_str")
                        .AppendColumns("value_int");
    auto results = GetTableClient().ReadTable("read_table", std::move(settings));

    auto cursor = results.GetNextResult();
    ASSERT_TRUE(cursor.has_value());
    AssertArePreFilledRows(std::move(*cursor), {1, 2, 3});

    EXPECT_FALSE(results.GetNextResult().has_value());
}

TYPED_UTEST(YdbExecuteTpl, IsQueryFromCache) {
    this->CreateTable("test_table", true);

    const ydb::Query kSelectQuery{R"(
    DECLARE $search_key AS String;

    SELECT key, value_str
    FROM test_table
    WHERE key = $search_key;
  )"};

    constexpr std::uint64_t kIterations = 16;

    for (std::uint64_t i = 0; i < kIterations; ++i) {
        auto params_builder = this->GetTableClient().GetBuilder();
        params_builder.Add("$search_key", std::string{"key1"});

        auto response = this->ExecuteWithBasicStats(ydb::OperationSettings{}, kSelectQuery, std::move(params_builder));
        EXPECT_TRUE(0 == i || response.IsFromServerQueryCache());
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

UTEST_F(YdbExecute, TransactionIsQueryFromCache) {
    const utils::statistics::Storage storage;

    CreateTable("test_table", true);
    auto transaction = GetTableClient().Begin("test_transaction");

    const ydb::Query kSelectQuery{R"(
    DECLARE $search_key AS String;

    SELECT key, value_str
    FROM test_table
    WHERE key = $search_key;
  )"};

    constexpr std::uint64_t kIterations = 16;

    for (std::uint64_t i = 0; i < kIterations; ++i) {
        ydb::QuerySettings query_settings;
        query_settings.collect_query_stats = NYdb::NTable::ECollectQueryStatsMode::Basic;

        auto params_builder = GetTableClient().GetBuilder();
        params_builder.Add("$search_key", std::string{"key1"});

        auto response = transaction.Execute(query_settings, {}, kSelectQuery, std::move(params_builder));
        EXPECT_TRUE(0 == i || response.IsFromServerQueryCache());
        AssertArePreFilledRows(response.GetSingleCursor(), {1});
    }
}

TYPED_UTEST(YdbExecuteTpl, PrepareQueryError) {
    this->CreateTable("prepare_error", true);

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, unexpected_column
        FROM prepare_error
        WHERE key = $search_key;
    )"};

    ydb::OperationSettings settings;
    settings.retries = 1;

    auto builder = this->GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key1"}));
    UASSERT_THROW(this->Execute(settings, query, std::move(builder)), ydb::YdbResponseError);
}

TYPED_UTEST(YdbExecuteTpl, ExecutePreparedError) {
    this->CreateTable("execute_prepared_error", true);

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT key, value_str
        FROM execute_prepared_error
        WHERE key = $search_key;
    )"};

    auto builder = this->GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", ydb::Utf8{"key1"}));
    UASSERT_THROW(this->Execute(ydb::OperationSettings{}, query, std::move(builder)), ydb::YdbResponseError);
}

TYPED_UTEST(YdbExecuteTpl, SimpleWriteReadBinary) {
    this->CreateTable("simple_write_read_binary", false);

    const ydb::Query insert_query{R"(
        --!syntax_v1
        DECLARE $value_str AS String;
        DECLARE $opt_value_str AS String?;

        INSERT INTO `simple_write_read_binary` (
            key,
            value_str
        )
        VALUES (
            "key1",
            $value_str
        ), (
            "key2",
            $opt_value_str
        )
    )"};
    const std::string insert_value_1 = std::string("\0test_value", 11);
    const std::string insert_value_2 = std::string("test_\0value", 11);
    ASSERT_EQ(insert_value_1[0], '\0') << "Must be binary string with leading zero byte";

    auto insert_builder = this->GetTableClient().GetBuilder();
    UASSERT_NO_THROW(insert_builder.Add("$value_str", insert_value_1));
    UASSERT_NO_THROW(insert_builder.Add("$opt_value_str", std::optional<std::string>(insert_value_2)));

    auto insert_response = this->Execute(ydb::OperationSettings{}, insert_query, std::move(insert_builder));

    ASSERT_EQ(insert_response.GetCursorCount(), 0);

    const ydb::Query select_query{R"(
        --DECLARE $key AS String;

        SELECT key, value_str
        FROM `simple_write_read_binary`;
        --WHERE key = "key1";
    )"};
    auto response = this->Execute(select_query);

    auto cursor = response.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 2);
    for (auto [i, row] : utils::enumerate(cursor)) {
        if (i == 0) {
            AssertNullableColumn(row, "key", "key1");
            AssertNullableColumn(row, "value_str", insert_value_1);
        } else {
            AssertNullableColumn(row, "key", "key2");
            AssertNullableColumn(row, "value_str", insert_value_2);
        }
    }
}

TYPED_UTEST(YdbExecuteTpl, GetSingleCursor) {
    auto response = this->Execute(ydb::Query{R"(
      SELECT 123 as num, "qwe" as str;
    )"});
    ASSERT_EQ(response.GetCursorCount(), 1);
    UASSERT_NO_THROW(response.GetSingleCursor());

    {
        auto cursor1 = response.GetSingleCursor();
        ASSERT_EQ(1, cursor1.RowsCount());
        ASSERT_EQ(2, cursor1.ColumnsCount());
        auto row = cursor1.GetFirstRow();
        EXPECT_EQ(123, row.template Get<int>(0));
        EXPECT_EQ("qwe", row.template Get<std::string>("str"));
    }

    // second time, check that response does not change
    {
        auto cursor2 = response.GetSingleCursor();
        ASSERT_EQ(1, cursor2.RowsCount());
        ASSERT_EQ(2, cursor2.ColumnsCount());
        auto row = cursor2.GetFirstRow();
        EXPECT_EQ(123, row.template Get<int>(0));
        EXPECT_EQ("qwe", row.template Get<std::string>("str"));
    }
}

TYPED_UTEST(YdbExecuteTpl, GetSingleCursorThrow) {
    auto response = this->Execute(ydb::Query{R"(
      SELECT 1;
      SELECT 1;
    )"});
    ASSERT_EQ(response.GetCursorCount(), 2);
    UASSERT_THROW(response.GetSingleCursor(), ydb::IgnoreResultsError);

    this->CreateTable("zero_cursor_throw", false);
    auto response1 = this->Execute(ydb::Query{R"(
        UPSERT INTO zero_cursor_throw (key, value_str, value_int)
        VALUES ("key", "value_str", 1);
    )"});
    ASSERT_EQ(response1.GetCursorCount(), 0);
    UASSERT_THROW(response1.GetSingleCursor(), ydb::EmptyResponseError);
}

TYPED_UTEST(YdbExecuteTpl, CursorThrow) {
    this->CreateTable("cursor_throw", true);
    const ydb::Query query{R"(
        SELECT *
        FROM cursor_throw
        WHERE key = "key1";
    )"};

    auto response = this->Execute(query);
    auto cursor = response.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    UASSERT_NO_THROW(cursor.GetFirstRow());
    UASSERT_THROW(cursor.begin(), ydb::EmptyResponseError);
    UASSERT_THROW(cursor.GetFirstRow(), ydb::EmptyResponseError);
}

USERVER_NAMESPACE_END
