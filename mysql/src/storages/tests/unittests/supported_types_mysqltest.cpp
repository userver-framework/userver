#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

#include <userver/formats/json/inline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

// TODO : dates
constexpr std::string_view kTableDefinitionNotNull = R"(
uint8_t TINYINT UNSIGNED NOT NULL,
int8_t TINYINT NOT NULL,
uint16_t SMALLINT UNSIGNED NOT NULL,
int16_t SMALLINT NOT NULL,
uint32_t INT UNSIGNED NOT NULL,
int32_t INT NOT NULL,
uint64_t BIGINT UNSIGNED NOT NULL,
int64_t BIGINT NOT NULL,
float_t FLOAT NOT NULL,
double_t DOUBLE NOT NULL,
string_t TEXT NOT NULL,
date_t DATE NOT NULL,
datetime_6_t DATETIME(6) NOT NULL,
json_t JSON NOT NULL
)";

constexpr std::string_view kTableDefinitionNullable = R"(
uint8_t TINYINT UNSIGNED,
int8_t TINYINT,
uint16_t SMALLINT UNSIGNED,
int16_t SMALLINT,
uint32_t INT UNSIGNED,
int32_t INT,
uint64_t BIGINT UNSIGNED,
int64_t BIGINT,
float_t FLOAT,
double_t DOUBLE,
string_t TEXT,
date_t DATE,
datetime_6_t DATETIME(6),
json_t JSON
)";

constexpr std::string_view kInsertQueryTemplate = R"(
INSERT INTO {}(
  uint8_t,
  int8_t,
  uint16_t,
  int16_t,
  uint32_t,
  int32_t,
  uint64_t,
  int64_t,
  float_t,
  double_t,
  string_t,
  date_t,
  datetime_6_t,
  json_t
) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
)";

constexpr std::string_view kSelectQueryTemplate = R"(
SELECT
  uint8_t,
  int8_t,
  uint16_t,
  int16_t,
  uint32_t,
  int32_t,
  uint64_t,
  int64_t,
  float_t,
  double_t,
  string_t,
  date_t,
  datetime_6_t,
  json_t
FROM {}
)";

struct AllSupportedTypesNotNull final {
  std::uint8_t uint8{};
  std::int8_t int8{};
  std::uint16_t uint16{};
  std::int16_t int16{};
  std::uint32_t uint32{};
  std::int32_t int32{};
  std::uint64_t uint64{};
  std::int64_t int64{};
  float float_t{};
  double double_t{};
  std::string string_t{};
  storages::mysql::Date date_t{};
  std::chrono::system_clock::time_point datetime_6_t{};
  formats::json::Value json_t{};
};

struct AllSupportedTypesNullable final {
  std::optional<std::uint8_t> uint8{};
  std::optional<std::int8_t> int8{};
  std::optional<std::uint16_t> uint16{};
  std::optional<std::int16_t> int16{};
  std::optional<std::uint32_t> uint32{};
  std::optional<std::int32_t> int32{};
  std::optional<std::uint64_t> uint64{};
  std::optional<std::int64_t> int64{};
  std::optional<float> float_t{};
  std::optional<double> double_t{};
  std::optional<std::string> string_t{};
  std::optional<storages::mysql::Date> date_t{};
  std::optional<std::chrono::system_clock::time_point> datetime_6_t{};
  std::optional<formats::json::Value> json_t{};
};

}  // namespace

UTEST(AllSupportedTypes, NotNull) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, kTableDefinitionNotNull};

  const auto insert_query = table.FormatWithTableName(kInsertQueryTemplate);
  const auto select_query = table.FormatWithTableName(kSelectQueryTemplate);

  const AllSupportedTypesNotNull row_to_insert{
      1,                                                        //
      2,                                                        //
      3,                                                        //
      4,                                                        //
      5,                                                        //
      6,                                                        //
      7,                                                        //
      8,                                                        //
      9.1f,                                                     //
      10.2,                                                     //
      "string_value_long_enough_to_avoid_sso",                  //
      storages::mysql::Date{std::chrono::system_clock::now()},  //
      std::chrono::system_clock::now(),                         //
      formats::json::MakeObject("key", 13)                      //
  };

  cluster->ExecuteDecompose(ClusterHostType::kPrimary, insert_query,
                            row_to_insert);

  const auto db_row = cluster->Execute(ClusterHostType::kPrimary, select_query)
                          .AsSingleRow<AllSupportedTypesNotNull>();

  EXPECT_EQ(row_to_insert.uint8, db_row.uint8);
  EXPECT_EQ(row_to_insert.int8, db_row.int8);
  EXPECT_EQ(row_to_insert.uint16, db_row.uint16);
  EXPECT_EQ(row_to_insert.int16, db_row.int16);
  EXPECT_EQ(row_to_insert.uint32, db_row.uint32);
  EXPECT_EQ(row_to_insert.int32, db_row.int32);
  EXPECT_EQ(row_to_insert.uint64, db_row.uint64);
  EXPECT_EQ(row_to_insert.int64, db_row.int64);
  EXPECT_FLOAT_EQ(row_to_insert.float_t, db_row.float_t);
  EXPECT_DOUBLE_EQ(row_to_insert.double_t, db_row.double_t);
  EXPECT_EQ(row_to_insert.string_t, db_row.string_t);
  EXPECT_EQ(row_to_insert.date_t, db_row.date_t);
  EXPECT_EQ(ToMariaDBPrecision(row_to_insert.datetime_6_t),
            db_row.datetime_6_t);
  EXPECT_EQ(row_to_insert.json_t, db_row.json_t);
}

UTEST(AllSupportedTypes, NullableWithNulls) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, kTableDefinitionNullable};

  const auto insert_query = table.FormatWithTableName(kInsertQueryTemplate);
  const auto select_query = table.FormatWithTableName(kSelectQueryTemplate);

  AllSupportedTypesNullable row_to_insert{};
  cluster->ExecuteDecompose(ClusterHostType::kPrimary, insert_query,
                            row_to_insert);

  const auto db_row = cluster->Execute(ClusterHostType::kPrimary, select_query)
                          .AsSingleRow<AllSupportedTypesNullable>();

  boost::pfr::for_each_field(
      db_row, [](const auto& f) { EXPECT_FALSE(f.has_value()); });
}

UTEST(AllSupportedTypes, NullableWithValues) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, kTableDefinitionNullable};

  const auto insert_query = table.FormatWithTableName(kInsertQueryTemplate);
  const auto select_query = table.FormatWithTableName(kSelectQueryTemplate);

  const AllSupportedTypesNullable row_to_insert{
      1,                                                        //
      2,                                                        //
      3,                                                        //
      4,                                                        //
      5,                                                        //
      6,                                                        //
      7,                                                        //
      8,                                                        //
      9.1f,                                                     //
      10.2,                                                     //
      "string_value_long_enough_to_avoid_sso",                  //
      storages::mysql::Date{std::chrono::system_clock::now()},  //
      std::chrono::system_clock::now(),                         //
      formats::json::MakeObject("key", 13)                      //
  };
  cluster->ExecuteDecompose(ClusterHostType::kPrimary, insert_query,
                            row_to_insert);

  const auto db_row = cluster->Execute(ClusterHostType::kPrimary, select_query)
                          .AsSingleRow<AllSupportedTypesNullable>();

  boost::pfr::for_each_field(db_row,
                             [](const auto& f) { EXPECT_TRUE(f.has_value()); });
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
