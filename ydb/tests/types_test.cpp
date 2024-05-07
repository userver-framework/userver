#include "test_utils.hpp"

#include <ydb-cpp-sdk/library/json/json_reader.h>
#include <ydb-cpp-sdk/library/json/writer/json_value.h>

#include <string>

#include <userver/utest/utest.hpp>

#include <userver/ydb/io/structs.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class TTypesYdbTestCase : public ydb::ClientFixtureBase {
 protected:
  TTypesYdbTestCase() { InitializeTable(); }

 private:
  void InitializeTable() {
    DoCreateTable(
        "different_types_test",
        NYdb::NTable::TTableBuilder()
            .AddNullableColumn("key", NYdb::EPrimitiveType::String)
            .AddNullableColumn("value_bool", NYdb::EPrimitiveType::Bool)
            .AddNullableColumn("value_int32", NYdb::EPrimitiveType::Int32)
            .AddNullableColumn("value_uint32", NYdb::EPrimitiveType::Uint32)
            .AddNullableColumn("value_int64", NYdb::EPrimitiveType::Int64)
            .AddNullableColumn("value_uint64", NYdb::EPrimitiveType::Uint64)
            .AddNullableColumn("value_double", NYdb::EPrimitiveType::Double)
            .AddNullableColumn("value_str", NYdb::EPrimitiveType::String)
            .AddNullableColumn("value_utf8", NYdb::EPrimitiveType::Utf8)
            .AddNullableColumn("value_ts", NYdb::EPrimitiveType::Timestamp)
            .AddNullableColumn("value_json", NYdb::EPrimitiveType::Json)
            .AddNullableColumn("value_json_doc",
                               NYdb::EPrimitiveType::JsonDocument)
            .SetPrimaryKeyColumn("key")
            .Build());

    const ydb::Query fill_query{
        R"(
          --!syntax_v1
          UPSERT INTO different_types_test (
              key, value_bool,
              value_int32, value_uint32, value_int64, value_uint64,
              value_double, value_str, value_utf8,
              value_ts,
              value_json,
              value_json_doc
          ) VALUES (
              "key", true,
              -32, 32, -64, 64,
              1.23, "str", "utf8",
              CAST(123 AS Timestamp),
              CAST('{"qwe": "asd", "zxc": 123}' AS Json),
              CAST('{"qwe": 123, "zxc": "asd"}' AS JsonDocument)
          ), (
              "key_null", null,
              null, null, null, null,
              null, null, null,
              null,
              null,
              null
          );
        )",
        ydb::Query::Name{"FillTable/different_types_test"},
    };

    GetTableClient().ExecuteDataQuery(fill_query);
  }
};

template <typename T>
void AssertNullableColumn(ydb::Row& r, const std::string& column, T exp) {
  auto value = r.Get<std::optional<T>>(column);
  ASSERT_TRUE(value);
  ASSERT_EQ(value.value(), exp);
}

template <typename T>
void AssertNullColumn(ydb::Row& r, const std::string& key) {
  auto value = r.Get<std::optional<T>>(key);
  ASSERT_FALSE(value);
}

template <typename T>
void DoTestPreparedRequestType(ydb::TableClient& table_client,
                               std::string type_str, std::string value_str,
                               T value) {
  const ydb::Query query{fmt::format(R"(
            DECLARE $search_value AS {};

            SELECT key
            FROM different_types_test
            WHERE {} = $search_value
            LIMIT 1;
        )",
                                     type_str.c_str(), value_str.c_str())};

  auto builder = table_client.GetBuilder();
  UASSERT_NO_THROW(builder.Add("$search_value", value));

  auto result = table_client.ExecuteDataQuery(ydb::OperationSettings{}, query,
                                              std::move(builder));
  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key"});
  }
}

}  // namespace

UTEST_F(TTypesYdbTestCase, ResponseValueType) {
  const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = $search_key;
    )"};

  auto builder = GetTableClient().GetBuilder();
  UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key"}));

  auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{},
                                                  query, std::move(builder));
  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key"});
    AssertNullableColumn(row, "value_bool", true);
    AssertNullableColumn(row, "value_int32", std::int32_t{-32});
    AssertNullableColumn(row, "value_uint32", std::uint32_t{32});
    AssertNullableColumn(row, "value_int64", std::int64_t{-64});
    AssertNullableColumn(row, "value_uint64", std::uint64_t{64});
    AssertNullableColumn(row, "value_double", 1.23);
    AssertNullableColumn(row, "value_str", std::string{"str"});
    AssertNullableColumn(row, "value_utf8", ydb::Utf8{"utf8"});
    AssertNullableColumn(
        row, "value_ts",
        std::chrono::system_clock::time_point(std::chrono::microseconds{123}));
    AssertNullableColumn(
        row, "value_json",
        formats::json::FromString(R"({"qwe": "asd", "zxc": 123})"));
    AssertNullableColumn(row, "value_json_doc",
                         ydb::JsonDocument(formats::json::FromString(
                             R"({"qwe":123,"zxc":"asd"})")));
  }
}

UTEST_F(TTypesYdbTestCase, ResponseNullValueType) {
  const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = $search_key;
    )"};

  auto builder = GetTableClient().GetBuilder();
  UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key_null"}));

  auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{},
                                                  query, std::move(builder));
  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn<std::string>(row, "key", "key_null");
    AssertNullColumn<bool>(row, "value_bool");
    AssertNullColumn<std::int32_t>(row, "value_int32");
    AssertNullColumn<std::uint32_t>(row, "value_uint32");
    AssertNullColumn<std::int64_t>(row, "value_int64");
    AssertNullColumn<std::uint64_t>(row, "value_uint64");
    AssertNullColumn<double>(row, "value_double");
    AssertNullColumn<std::string>(row, "value_str");
    AssertNullColumn<ydb::Utf8>(row, "value_utf8");
    AssertNullColumn<std::chrono::system_clock::time_point>(row, "value_ts");
    AssertNullColumn<formats::json::Value>(row, "value_json");
    AssertNullColumn<ydb::JsonDocument>(row, "value_json_doc");
  }
}

UTEST_F(TTypesYdbTestCase, PreparedRequestType) {
  DoTestPreparedRequestType(GetTableClient(), "Bool", "value_bool", true);
  DoTestPreparedRequestType(GetTableClient(), "Int32", "value_int32",
                            std::int32_t{-32});
  DoTestPreparedRequestType(GetTableClient(), "Uint32", "value_uint32",
                            std::uint32_t{32});
  DoTestPreparedRequestType(GetTableClient(), "Int64", "value_int64",
                            std::int64_t{-64});
  DoTestPreparedRequestType(GetTableClient(), "Uint64", "value_uint64",
                            std::uint64_t{64});
  DoTestPreparedRequestType<double>(GetTableClient(), "Double", "value_double",
                                    1.23);
  DoTestPreparedRequestType(GetTableClient(), "String", "value_str",
                            std::string{"str"});
  DoTestPreparedRequestType(GetTableClient(), "Utf8", "value_utf8",
                            ydb::Utf8{"utf8"});
  DoTestPreparedRequestType(
      GetTableClient(), "Timestamp", "value_ts",
      std::chrono::system_clock::time_point{std::chrono::microseconds{123}});
}

UTEST_F(TTypesYdbTestCase, PreparedUpsertTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_bool AS Bool;
        DECLARE $data_int32 AS Int32;
        DECLARE $data_uint32 AS Uint32;
        DECLARE $data_int64 AS Int64;
        DECLARE $data_uint64 AS Uint64;
        DECLARE $data_double AS Double;
        DECLARE $data_str AS String;
        DECLARE $data_utf8 AS Utf8;
        DECLARE $data_ts AS Timestamp;
        DECLARE $data_json AS String;
        DECLARE $data_json_doc AS String;

        UPSERT INTO different_types_test (
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        ) VALUES (
            $search_key,
            $data_bool,
            $data_int32,
            $data_uint32,
            $data_int64,
            $data_uint64,
            $data_double,
            $data_str,
            $data_utf8,
            $data_ts,
            CAST($data_json AS Json),
            CAST($data_json_doc AS JsonDocument)
        );
    )"};

  auto builder = GetTableClient().GetBuilder();
  UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key_new_filled"}));
  UASSERT_NO_THROW(builder.Add("$data_bool", false));
  UASSERT_NO_THROW(builder.Add("$data_int32", std::int32_t{-321}));
  UASSERT_NO_THROW(builder.Add("$data_uint32", std::uint32_t{321}));
  UASSERT_NO_THROW(builder.Add("$data_int64", std::int64_t{-641}));
  UASSERT_NO_THROW(builder.Add("$data_uint64", std::uint64_t{641}));
  UASSERT_NO_THROW(builder.Add("$data_uint64", std::uint64_t{641}));
  UASSERT_NO_THROW(builder.Add("$data_double", 1.234));
  UASSERT_NO_THROW(builder.Add("$data_str", std::string{"string"}));
  UASSERT_NO_THROW(builder.Add("$data_utf8", ydb::Utf8{"utf8"}));
  UASSERT_NO_THROW(builder.Add(
      "$data_ts",
      std::chrono::system_clock::time_point{std::chrono::microseconds{12345}}));
  UASSERT_NO_THROW(builder.Add("$data_json", std::string{R"({"qwe":123})"}));
  UASSERT_NO_THROW(
      builder.Add("$data_json_doc", std::string{R"({"qwe":456})"}));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = "key_new_filled";
    )"});

  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key_new_filled"});
    AssertNullableColumn(row, "value_bool", false);
    AssertNullableColumn(row, "value_int32", std::int32_t{-321});
    AssertNullableColumn(row, "value_uint32", std::uint32_t{321});
    AssertNullableColumn(row, "value_int64", std::int64_t{-641});
    AssertNullableColumn(row, "value_uint64", std::uint64_t{641});
    AssertNullableColumn(row, "value_double", 1.234);
    AssertNullableColumn(row, "value_str", std::string{"string"});
    AssertNullableColumn(row, "value_utf8", ydb::Utf8{"utf8"});
    AssertNullableColumn(row, "value_ts",
                         std::chrono::system_clock::time_point(
                             std::chrono::microseconds{12345}));
    AssertNullableColumn(row, "value_json",
                         formats::json::FromString(R"({"qwe": 123})"));
    AssertNullableColumn(
        row, "value_json_doc",
        ydb::JsonDocument(formats::json::FromString(R"({"qwe":456})")));
  }
}

UTEST_F(TTypesYdbTestCase, PreparedStructUpsertTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $items AS List<Struct<'key': String, 'value_bool': Bool, 'value_int32': Int32,'value_uint32': UInt32, 'value_int64': Int64, 'value_uint64': UInt64, 'value_double': Double, 'value_str': String, 'value_utf8': Utf8, 'value_ts': Timestamp>>;

        UPSERT INTO different_types_test
        SELECT * FROM AS_TABLE($items);
    )"};

  auto builder = GetTableClient().GetBuilder();
  auto row = ydb::InsertRow{
      ydb::InsertColumn{"key", std::string{"key_new_filled_struct"}},
      ydb::InsertColumn{"value_bool", false},
      ydb::InsertColumn{"value_int32", std::int32_t{-321}},
      ydb::InsertColumn{"value_uint32", std::uint32_t{321}},
      ydb::InsertColumn{"value_int64", std::int64_t{-641}},
      ydb::InsertColumn{"value_uint64", std::uint64_t{641}},
      ydb::InsertColumn{"value_double", 1.234},
      ydb::InsertColumn{"value_str", std::string{"string"}},
      ydb::InsertColumn{"value_utf8", ydb::Utf8{"utf8"}},
      ydb::InsertColumn{"value_ts", std::chrono::system_clock::time_point(
                                        std::chrono::microseconds{12345})},
  };
  std::vector<ydb::InsertRow> rows{row};
  UASSERT_NO_THROW(builder.Add("$items", rows));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = "key_new_filled_struct";
    )"});
  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key_new_filled_struct"});
    AssertNullableColumn(row, "value_bool", false);
    AssertNullableColumn(row, "value_int32", std::int32_t{-321});
    AssertNullableColumn(row, "value_uint32", std::uint32_t{321});
    AssertNullableColumn(row, "value_int64", std::int64_t{-641});
    AssertNullableColumn(row, "value_uint64", std::uint64_t{641});
    AssertNullableColumn(row, "value_double", 1.234);
    AssertNullableColumn(row, "value_str", std::string{"string"});
    AssertNullableColumn(row, "value_utf8", ydb::Utf8{"utf8"});
    AssertNullableColumn(row, "value_ts",
                         std::chrono::system_clock::time_point(
                             std::chrono::microseconds{12345}));
  }
}

UTEST_F(TTypesYdbTestCase, PreparedUpsertNullableTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_bool AS Bool?;
        DECLARE $data_int32 AS Int32?;
        DECLARE $data_uint32 AS Uint32?;
        DECLARE $data_int64 AS Int64?;
        DECLARE $data_uint64 AS Uint64?;
        DECLARE $data_double AS Double?;
        DECLARE $data_str AS String?;
        DECLARE $data_utf8 AS Utf8?;
        DECLARE $data_ts AS Timestamp?;
        DECLARE $data_json AS String?;
        DECLARE $data_json_doc AS String?;

        UPSERT INTO different_types_test (
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        ) VALUES (
            $search_key,
            $data_bool,
            $data_int32,
            $data_uint32,
            $data_int64,
            $data_uint64,
            $data_double,
            $data_str,
            $data_utf8,
            $data_ts,
            CAST($data_json AS Json),
            CAST($data_json_doc AS JsonDocument)
        );
    )"};

  auto builder = GetTableClient().GetBuilder();

  UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key_new_nullable"}));
  UASSERT_NO_THROW(builder.Add("$data_bool", std::optional{false}));
  UASSERT_NO_THROW(
      builder.Add("$data_int32", std::optional{std::int32_t{-321}}));
  UASSERT_NO_THROW(
      builder.Add("$data_uint32", std::optional{std::uint32_t{321}}));
  UASSERT_NO_THROW(
      builder.Add("$data_int64", std::optional{std::int64_t{-641}}));
  UASSERT_NO_THROW(
      builder.Add("$data_uint64", std::optional{std::uint64_t{641}}));
  UASSERT_NO_THROW(builder.Add("$data_double", std::optional{1.234}));
  UASSERT_NO_THROW(builder.Add(
      "$data_str", std::optional{std::string("\0byte_string", 12)}));
  UASSERT_NO_THROW(builder.Add("$data_utf8", std::optional{ydb::Utf8{"utf8"}}));
  UASSERT_NO_THROW(builder.Add(
      "$data_ts", std::optional{std::chrono::system_clock::time_point{
                      std::chrono::microseconds{12345}}}));
  UASSERT_NO_THROW(
      builder.Add("$data_json", std::optional{std::string{R"({"qwe":123})"}}));
  UASSERT_NO_THROW(builder.Add("$data_json_doc",
                               std::optional{std::string{R"({"qwe":456})"}}));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = "key_new_nullable";
    )"});

  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key_new_nullable"});
    AssertNullableColumn(row, "value_bool", false);
    AssertNullableColumn(row, "value_int32", std::int32_t{-321});
    AssertNullableColumn(row, "value_uint32", std::uint32_t{321});
    AssertNullableColumn(row, "value_int64", std::int64_t{-641});
    AssertNullableColumn(row, "value_uint64", std::uint64_t{641});
    AssertNullableColumn(row, "value_double", 1.234);
    AssertNullableColumn(row, "value_str", std::string{"\0byte_string", 12});
    AssertNullableColumn(row, "value_utf8", ydb::Utf8{"utf8"});
    AssertNullableColumn(row, "value_ts",
                         std::chrono::system_clock::time_point(
                             std::chrono::microseconds{12345}));
    AssertNullableColumn(row, "value_json",
                         formats::json::FromString(R"({"qwe": 123})"));
    AssertNullableColumn(
        row, "value_json_doc",
        ydb::JsonDocument(formats::json::FromString(R"({"qwe":456})")));
  }
}

UTEST_F(TTypesYdbTestCase, PreparedStructUpsertNullableTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $items AS List<Struct<'key': String, 'value_bool': Bool?, 'value_int32': Int32?, 'value_uint32': UInt32?, 'value_int64': Int64?, 'value_uint64': UInt64?, 'value_double': Double?, 'value_str': String?, 'value_utf8': Utf8?, 'value_ts': Timestamp?>>;

        UPSERT INTO different_types_test
        SELECT * FROM AS_TABLE($items);
    )"};

  auto builder = GetTableClient().GetBuilder();
  auto row = ydb::InsertRow{
      ydb::InsertColumn{"key", std::string{"key_new_nullable_struct"}},
      ydb::InsertColumn{"value_bool", std::optional{false}},
      ydb::InsertColumn{"value_int32", std::optional{std::int32_t{-321}}},
      ydb::InsertColumn{"value_uint32", std::optional{std::uint32_t{321}}},
      ydb::InsertColumn{"value_int64", std::optional{std::int64_t{-641}}},
      ydb::InsertColumn{"value_uint64", std::optional{std::uint64_t{641}}},
      ydb::InsertColumn{"value_double", std::optional{1.234}},
      ydb::InsertColumn{"value_str", std::optional{std::string("string")}},
      ydb::InsertColumn{"value_utf8", std::optional{ydb::Utf8{"utf8"}}},
      ydb::InsertColumn{"value_ts",
                        std::optional{std::chrono::system_clock::time_point{
                            std::chrono::microseconds{12345}}}},
  };
  std::vector<ydb::InsertRow> rows{row};
  UASSERT_NO_THROW(builder.Add("$items", rows));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = "key_new_nullable_struct";
    )"});

  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn(row, "key", std::string{"key_new_nullable_struct"});
    AssertNullableColumn(row, "value_bool", false);
    AssertNullableColumn(row, "value_int32", std::int32_t{-321});
    AssertNullableColumn(row, "value_uint32", std::uint32_t{321});
    AssertNullableColumn(row, "value_int64", std::int64_t{-641});
    AssertNullableColumn(row, "value_uint64", std::uint64_t{641});
    AssertNullableColumn(row, "value_double", 1.234);
    AssertNullableColumn(row, "value_str", std::string{"string"});
    AssertNullableColumn(row, "value_utf8", ydb::Utf8{"utf8"});
    AssertNullableColumn(row, "value_ts",
                         std::chrono::system_clock::time_point(
                             std::chrono::microseconds{12345}));
  }
}

UTEST_F(TTypesYdbTestCase, PreparedUpsertNullTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_bool AS Bool?;
        DECLARE $data_int32 AS Int32?;
        DECLARE $data_uint32 AS Uint32?;
        DECLARE $data_int64 AS Int64?;
        DECLARE $data_uint64 AS Uint64?;
        DECLARE $data_double AS Double?;
        DECLARE $data_str AS String?;
        DECLARE $data_utf8 AS Utf8?;
        DECLARE $data_ts AS Timestamp?;
        DECLARE $data_json AS String?;
        DECLARE $data_json_doc AS String?;

        UPSERT INTO different_types_test (
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        ) VALUES (
            $search_key,
            $data_bool,
            $data_int32,
            $data_uint32,
            $data_int64,
            $data_uint64,
            $data_double,
            $data_str,
            $data_utf8,
            $data_ts,
            CAST($data_json AS Json),
            CAST($data_json_doc AS JsonDocument)
        );
    )"};

  auto builder = GetTableClient().GetBuilder();

  UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key_new_nulls"}));

  UASSERT_NO_THROW(builder.Add("$data_bool", std::optional<bool>{}));
  UASSERT_NO_THROW(builder.Add("$data_int32", std::optional<std::int32_t>{}));
  UASSERT_NO_THROW(builder.Add("$data_uint32", std::optional<std::uint32_t>{}));
  UASSERT_NO_THROW(builder.Add("$data_int64", std::optional<std::int64_t>{}));
  UASSERT_NO_THROW(builder.Add("$data_uint64", std::optional<std::uint64_t>{}));
  UASSERT_NO_THROW(builder.Add("$data_double", std::optional<double>{}));
  UASSERT_NO_THROW(builder.Add("$data_str", std::optional<std::string>{}));
  UASSERT_NO_THROW(builder.Add("$data_utf8", std::optional<ydb::Utf8>{}));
  UASSERT_NO_THROW(builder.Add(
      "$data_ts", std::optional<std::chrono::system_clock::time_point>{}));
  UASSERT_NO_THROW(builder.Add("$data_json", std::optional<std::string>{}));
  UASSERT_NO_THROW(builder.Add("$data_json_doc", std::optional<std::string>{}));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts,
            value_json,
            value_json_doc
        FROM different_types_test
        WHERE key = "key_new_nulls";
    )"});

  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn<std::string>(row, "key", "key_new_nulls");
    AssertNullColumn<bool>(row, "value_bool");
    AssertNullColumn<std::int32_t>(row, "value_int32");
    AssertNullColumn<std::uint32_t>(row, "value_uint32");
    AssertNullColumn<std::int64_t>(row, "value_int64");
    AssertNullColumn<std::uint64_t>(row, "value_uint64");
    AssertNullColumn<double>(row, "value_double");
    AssertNullColumn<std::string>(row, "value_str");
    AssertNullColumn<ydb::Utf8>(row, "value_utf8");
    AssertNullColumn<std::chrono::system_clock::time_point>(row, "value_ts");
    AssertNullColumn<formats::json::Value>(row, "value_json");
    AssertNullColumn<ydb::JsonDocument>(row, "value_json_doc");
  }
}

UTEST_F(TTypesYdbTestCase, PreparedUpsertNullStructTypes) {
  const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $items AS List<Struct<'key': String, 'value_bool': Bool?, 'value_int32': Int32?, 'value_uint32': UInt32?, 'value_int64': Int64?, 'value_uint64': UInt64?, 'value_double': Double?, 'value_str': String?, 'value_utf8': Utf8?, 'value_ts': Timestamp?>>;

        UPSERT INTO different_types_test
        SELECT * FROM AS_TABLE($items);
    )"};

  auto builder = GetTableClient().GetBuilder();
  auto row = ydb::InsertRow{
      ydb::InsertColumn{"key", std::string{"key_new_nulls_struct"}},
      ydb::InsertColumn{"value_bool", std::optional<bool>{}},
      ydb::InsertColumn{"value_int32", std::optional<std::int32_t>{}},
      ydb::InsertColumn{"value_uint32", std::optional<std::uint32_t>{}},
      ydb::InsertColumn{"value_int64", std::optional<std::int64_t>{}},
      ydb::InsertColumn{"value_uint64", std::optional<std::uint64_t>{}},
      ydb::InsertColumn{"value_double", std::optional<double>{}},
      ydb::InsertColumn{"value_str", std::optional<std::string>{}},
      ydb::InsertColumn{"value_utf8", std::optional<ydb::Utf8>{}},
      ydb::InsertColumn{
          "value_ts", std::optional<std::chrono::system_clock::time_point>{}}};

  std::vector<ydb::InsertRow> rows{row};
  UASSERT_NO_THROW(builder.Add("$items", rows));

  UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(
      ydb::OperationSettings{}, query, std::move(builder)));

  auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_bool,
            value_int32,
            value_uint32,
            value_int64,
            value_uint64,
            value_double,
            value_str,
            value_utf8,
            value_ts
        FROM different_types_test
        WHERE key = "key_new_nulls_struct";
    )"});

  auto cursor = result.GetSingleCursor();
  ASSERT_EQ(cursor.size(), 1);
  for (auto row : cursor) {
    AssertNullableColumn<std::string>(row, "key", "key_new_nulls_struct");
    AssertNullColumn<bool>(row, "value_bool");
    AssertNullColumn<std::int32_t>(row, "value_int32");
    AssertNullColumn<std::uint32_t>(row, "value_uint32");
    AssertNullColumn<std::int64_t>(row, "value_int64");
    AssertNullColumn<std::uint64_t>(row, "value_uint64");
    AssertNullColumn<double>(row, "value_double");
    AssertNullColumn<std::string>(row, "value_str");
    AssertNullColumn<ydb::Utf8>(row, "value_utf8");
    AssertNullColumn<std::chrono::system_clock::time_point>(row, "value_ts");
  }
}
USERVER_NAMESPACE_END
