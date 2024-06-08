#include <userver/utest/utest.hpp>

#include "test_utils.hpp"

#include <string>

USERVER_NAMESPACE_BEGIN

namespace {

class TYdbRowGetTestCase : public ydb::ClientFixtureBase {
 protected:
  TYdbRowGetTestCase() { InitializeTable(); }

 private:
  void InitializeTable() {
    // At this moment we could not create NOT NULL NON KEY fields
    DoCreateTable(
        "null_not_null_table",
        NYdb::NTable::TTableBuilder()
            .AddNonNullableColumn("key", NYdb::EPrimitiveType::Uint32)
            .AddNullableColumn("nullable_bool", NYdb::EPrimitiveType::Bool)
            .AddNullableColumn("nullable_str", NYdb::EPrimitiveType::String)
            .SetPrimaryKeyColumn("key")
            .Build());

    const ydb::Query fill_query{
        R"(
          --!syntax_v1
          UPSERT INTO null_not_null_table (
            key, nullable_bool, nullable_str
          ) VALUES (
            123, false, "helloworld"
          ), (
            321, null, "hi"
          );
        )",
        ydb::Query::Name{"FillTable/null_not_null_table"},
    };

    GetTableClient().ExecuteDataQuery(fill_query);
  }
};

using TYdbRowGetTestCaseDeathTest = TYdbRowGetTestCase;

ydb::Cursor GetCursor(ydb::TableClient& table_client) {
  static const ydb::Query query{R"(
    DECLARE $search_key AS Uint32;

    SELECT
      *
    FROM null_not_null_table
    WHERE key = $search_key;
  )"};

  auto builder = table_client.GetBuilder();
  builder.Add("$search_key", std::uint32_t{321});

  auto response = table_client.ExecuteDataQuery(ydb::OperationSettings{}, query,
                                                std::move(builder));
  auto cursor = response.GetSingleCursor();
  if (cursor.size() != 1) {
    throw std::runtime_error{"cursor size is not 1"};
  }
  return cursor;
};

}  // namespace

UTEST_F(TYdbRowGetTestCase, MismatchedType) {
  UASSERT_NO_THROW(
      GetCursor(GetTableClient()).GetFirstRow().Get<std::uint32_t>("key"));
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::optional<bool>>("nullable_bool"));
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::optional<std::uint32_t>>("key"));
  EXPECT_THROW(
      GetCursor(GetTableClient()).GetFirstRow().Get<std::string>("key"),
      yexception);
  EXPECT_THROW(GetCursor(GetTableClient())
                   .GetFirstRow()
                   .Get<std::optional<std::string>>("key"),
               yexception);
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::optional<std::uint64_t>>("nullable_bool"));
}

UTEST_F(TYdbRowGetTestCase, NullField) {
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::optional<bool>>("nullable_bool"));
  EXPECT_THROW(
      GetCursor(GetTableClient()).GetFirstRow().Get<bool>("nullable_bool"),
      yexception);
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::optional<std::string>>("nullable_str"));
  UASSERT_NO_THROW(GetCursor(GetTableClient())
                       .GetFirstRow()
                       .Get<std::string>("nullable_str"));
}

UTEST_F_DEATH(TYdbRowGetTestCaseDeathTest, TwiceGet) {
#ifdef NDEBUG
  GTEST_SKIP() << "Columns in row is not checked in release";
#endif
  auto cursor = GetCursor(GetTableClient());
  auto row = cursor.GetFirstRow();
  UASSERT_NO_THROW(row.Get<std::optional<bool>>("nullable_bool"));
  UEXPECT_DEATH(row.Get<std::optional<bool>>("nullable_bool"),
                "It is allowed to take the value of column only once. Index 1 "
                "is already consumed");
}

USERVER_NAMESPACE_END
