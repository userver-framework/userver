#pragma once

#include <fmt/format.h>
#include <gmock/gmock.h>

#include <userver/utils/impl/source_location.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

class YdbSmallTableTest : public ydb::ClientFixtureBase {
 protected:
  void CreateTable(std::string_view table_name, bool fill_table) {
    DoCreateTable(
        table_name,
        NYdb::NTable::TTableBuilder()
            .AddNullableColumn("key", NYdb::EPrimitiveType::String)
            .AddNullableColumn("value_str", NYdb::EPrimitiveType::String)
            .AddNullableColumn("value_int", NYdb::EPrimitiveType::Int32)
            .SetPrimaryKeyColumn("key")
            .AddAsyncSecondaryIndex("value_idx", "value_int")
            .Build());

    if (fill_table) {
      const ydb::Query fill_query{
          fmt::format(R"(
            UPSERT INTO {} (key, value_str, value_int)
            VALUES ("key1", "value1", 1), ("key2", "value2", 2), ("key3", "value3", 3);
          )",
                      table_name),
          ydb::Query::Name{fmt::format("FillTable/{}", table_name)},
      };
      GetTableClient().ExecuteDataQuery(fill_query);
    }
  }
};

/// [struct sample]
namespace tests {

struct RowValue {
  static constexpr ydb::StructMemberNames kYdbMemberNames{};

  std::string key;
  std::string value_str;
  std::int32_t value_int;
};

}  // namespace tests
/// [struct sample]

inline const std::vector<tests::RowValue> kPreFilledRows = {
    {"key1", "value1", 1},
    {"key2", "value2", 2},
    {"key3", "value3", 3},
};

inline std::string ToString(const utils::impl::SourceLocation& location) {
  return fmt::format("at {}, {}:{}", location.GetFunctionName(),
                     location.GetFileName(), location.GetLineString());
}

template <typename T>
void AssertNullableColumn(ydb::Row& row, std::string_view column_name,
                          const T& expected,
                          const utils::impl::SourceLocation& location =
                              utils::impl::SourceLocation::Current()) {
  using ExpectedType = std::decay_t<decltype(expected)>;
  using RowType =
      std::conditional_t<std::is_convertible_v<ExpectedType, std::string_view>,
                         std::string, ExpectedType>;
  UEXPECT_NO_THROW(
      EXPECT_EQ(row.Get<std::optional<RowType>>(column_name), expected)
      << ToString(location))
      << ToString(location);
}

inline void AssertIsPreFilledRow(ydb::Row row, std::size_t index,
                                 const utils::impl::SourceLocation& location =
                                     utils::impl::SourceLocation::Current()) {
  AssertNullableColumn(row, "key", kPreFilledRows[index - 1].key, location);
  AssertNullableColumn(row, "value_str", kPreFilledRows[index - 1].value_str,
                       location);
}

inline auto AssertArePreFilledRows(ydb::Cursor cursor,
                                   std::vector<std::size_t> indexes,
                                   const utils::impl::SourceLocation& location =
                                       utils::impl::SourceLocation::Current()) {
  ASSERT_THAT(cursor, testing::SizeIs(indexes.size()))
      << "expected " << indexes.size() << " rows in cursor "
      << ToString(location);

  for (auto [pos, row] : utils::enumerate(cursor)) {
    AssertIsPreFilledRow(std::move(row), indexes[pos], location);
  }
}

inline const ydb::Query kSelectAllRows{R"(
--!syntax_v1
SELECT key, value_str, value_int
FROM test_table
ORDER BY key
)"};

USERVER_NAMESPACE_END
