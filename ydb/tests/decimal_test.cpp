#include "test_utils.hpp"

#include <userver/utest/utest.hpp>

#include <userver/decimal64/decimal64.hpp>
#include <userver/ydb/io/supported_types.hpp>
#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class TDecimalYdbTestCase : public ydb::ClientFixtureBase {
protected:
    TDecimalYdbTestCase() { InitializeTable(); }

private:
    void InitializeTable() {
        DoCreateTable(
            "decimal_test",
            NYdb::NTable::TTableBuilder()
                .AddNullableColumn("key", NYdb::EPrimitiveType::String)
                .AddNullableColumn("value_decimal", NYdb::TDecimalType{/*precision=*/22, /*scale=*/9})
                .SetPrimaryKeyColumn("key")
                .Build()
        );

        const ydb::Query fill_query{
            R"(
              --!syntax_v1
              UPSERT INTO decimal_test (
                  key, value_decimal
              ) VALUES (
                  "key", CAST("123.456789" AS Decimal(22, 9))
              ), (
                  "key_null", null
              );
            )",
            ydb::Query::Name{"FillTable/decimal_test"},
        };

        GetTableClient().ExecuteDataQuery(fill_query);
    }
};

// The test fixture table uses Decimal(22, 9) To verify that `ydb::Decimal`
// correctly handles non-default precision/scale at the serialization level,
// this fixture uses prepared parameters declared as Decimal(35, 18).
using TDecimalWideYdbTestCase = ydb::ClientFixtureBase;

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

}  // namespace

UTEST_F(TDecimalYdbTestCase, ResponseValueType) {
    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = $search_key;
    )"};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key"}));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key"});
        AssertNullableColumn(row, "value_decimal", ydb::Decimal{"123.456789", 22, 9});
    }
}

UTEST_F(TDecimalYdbTestCase, PreparedRequestType) {
    const ydb::Query query{R"(
        DECLARE $search_value AS Decimal(22, 9);

        SELECT key
        FROM decimal_test
        WHERE value_decimal = $search_value
        LIMIT 1;
    )"};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_value", ydb::Decimal{"123.456789", 22, 9}));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key"});
    }
}

UTEST_F(TDecimalYdbTestCase, ResponseValueTypeDecimal64) {
    using Money = decimal64::Decimal<9>;

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = $search_key;
    )"};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key"}));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key"});
        AssertNullableColumn(row, "value_decimal", Money{"123.456789"});
    }
}

UTEST_F(TDecimalYdbTestCase, ResponseNullValueTypeDecimal64) {
    using Money = decimal64::Decimal<9>;

    const ydb::Query query{R"(
        DECLARE $search_key AS String;

        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = $search_key;
    )"};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_key", std::string{"key_null"}));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn<std::string>(row, "key", "key_null");
        AssertNullColumn<Money>(row, "value_decimal");
    }
}

UTEST_F(TDecimalYdbTestCase, PreparedRequestTypeDecimal64) {
    using Money = decimal64::Decimal<9>;

    const ydb::Query query{R"(
        DECLARE $search_value AS Decimal(22, 9);

        SELECT key
        FROM decimal_test
        WHERE value_decimal = $search_value
        LIMIT 1;
    )"};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$search_value", Money{"123.456789"}));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key"});
    }
}

UTEST_F(TDecimalYdbTestCase, PreparedUpsertTypeDecimal64) {
    using Money = decimal64::Decimal<9>;

    const ydb::Query upsert_query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_decimal AS Decimal(22, 9);

        UPSERT INTO decimal_test (
            key,
            value_decimal
        ) VALUES (
            $search_key,
            $data_decimal
        );
    )"};

    auto upsert_builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(upsert_builder.Add("$search_key", std::string{"key_new_decimal64"}));
    UASSERT_NO_THROW(upsert_builder.Add("$data_decimal", Money{"-987.654321"}));

    UASSERT_NO_THROW(
        GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, upsert_query, std::move(upsert_builder))
    );

    auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = "key_new_decimal64";
    )"});

    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key_new_decimal64"});
        AssertNullableColumn(row, "value_decimal", Money{"-987.654321"});
    }
}

UTEST_F(TDecimalYdbTestCase, PreparedUpsertNullTypeDecimal64) {
    using Money = decimal64::Decimal<9>;

    const ydb::Query upsert_query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_decimal AS Decimal(22, 9)?;

        UPSERT INTO decimal_test (
            key,
            value_decimal
        ) VALUES (
            $search_key,
            $data_decimal
        );
    )"};

    auto upsert_builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(upsert_builder.Add("$search_key", std::string{"key_new_decimal64_null"}));
    UASSERT_NO_THROW(upsert_builder.Add("$data_decimal", std::optional<Money>{}));

    UASSERT_NO_THROW(
        GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, upsert_query, std::move(upsert_builder))
    );

    auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = "key_new_decimal64_null";
    )"});

    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn<std::string>(row, "key", "key_new_decimal64_null");
        AssertNullColumn<Money>(row, "value_decimal");
    }
}

UTEST_F(TDecimalYdbTestCase, InsertRowUpsertType) {
    const ydb::Query query{R"(
        --!syntax_v1
        DECLARE $items AS List<
            Struct<
                'key': String,
                'value_decimal': Decimal(22, 9)
            >
        >;

        UPSERT INTO decimal_test
        SELECT * FROM AS_TABLE($items);
    )"};

    auto builder = GetTableClient().GetBuilder();
    auto row = ydb::InsertRow{
        ydb::InsertColumn{"key", std::string{"key_new_insertrow"}},
        ydb::InsertColumn{"value_decimal", ydb::Decimal{"42.000000001", 22, 9}},
    };
    std::vector<ydb::InsertRow> rows{row};
    UASSERT_NO_THROW(builder.Add("$items", rows));

    UASSERT_NO_THROW(GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder)));

    auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = "key_new_insertrow";
    )"});

    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key_new_insertrow"});
        AssertNullableColumn(row, "value_decimal", ydb::Decimal{"42.000000001", 22, 9});
    }
}

UTEST_F(TDecimalYdbTestCase, PreparedUpsertType) {
    const ydb::Query upsert_query{R"(
        --!syntax_v1
        DECLARE $search_key AS String;
        DECLARE $data_decimal AS Decimal(22, 9);

        UPSERT INTO decimal_test (
            key,
            value_decimal
        ) VALUES (
            $search_key,
            $data_decimal
        );
    )"};

    auto upsert_builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(upsert_builder.Add("$search_key", std::string{"key_new_decimal"}));
    UASSERT_NO_THROW(upsert_builder.Add("$data_decimal", ydb::Decimal{"-987.654321", 22, 9}));

    UASSERT_NO_THROW(
        GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, upsert_query, std::move(upsert_builder))
    );

    auto result = GetTableClient().ExecuteDataQuery(ydb::Query{R"(
        SELECT
            key,
            value_decimal
        FROM decimal_test
        WHERE key = "key_new_decimal";
    )"});

    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "key", std::string{"key_new_decimal"});
        AssertNullableColumn(row, "value_decimal", ydb::Decimal{"-987.654321", 22, 9});
    }
}

// Verifies that `ydb::Decimal` correctly serializes non-default precision/scale.
// The test fixture table uses Decimal(22, 9), so this test uses a prepared
// parameter declared as `Decimal(35, 18)` and selects it back without storing it
// in the fixture table. This checks that precision/scale round-trip through the SDK.
UTEST_F(TDecimalWideYdbTestCase, RuntimePrecisionScale) {
    const ydb::Query query{R"(
        DECLARE $v AS Decimal(35, 18);

        SELECT $v AS value_decimal;
    )"};

    const ydb::Decimal input{"-12345678901234567.123456789012345678", 35, 18};

    auto builder = GetTableClient().GetBuilder();
    UASSERT_NO_THROW(builder.Add("$v", input));

    auto result = GetTableClient().ExecuteDataQuery(ydb::OperationSettings{}, query, std::move(builder));
    auto cursor = result.GetSingleCursor();
    ASSERT_EQ(cursor.size(), 1);
    for (auto row : cursor) {
        AssertNullableColumn(row, "value_decimal", input);
    }
}

USERVER_NAMESPACE_END
