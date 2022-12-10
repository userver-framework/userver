#include "utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr std::string_view kTableDefinition = R"(
(
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
  datetime_6_t DATETIME(6)
)
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
  datetime_6_t
) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
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
  datetime_6_t
FROM {}
)";

struct AllSupportedTypes final {
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
  std::chrono::system_clock::time_point datetime_6_t{};
};

}  // namespace

UTEST(AllSupportedTypes, Basic) {
  ClusterWrapper cluster{};

  TmpTable table{cluster, kTableDefinition};

  const auto insert_query = table.FormatWithTableName(kInsertQueryTemplate);
  const auto select_query = table.FormatWithTableName(kSelectQueryTemplate);

  const AllSupportedTypes row_to_insert{
      1,                                        //
      2,                                        //
      3,                                        //
      4,                                        //
      5,                                        //
      6,                                        //
      7,                                        //
      8,                                        //
      9.1f,                                     //
      10.2,                                     //
      "string_value_long_enough_to_avoid_sso",  //
      std::chrono::system_clock::now()};

  cluster->InsertOne(cluster.GetDeadline(), insert_query, row_to_insert);

  const auto row = cluster
                       ->Select(ClusterHostType::kMaster, cluster.GetDeadline(),
                                select_query)
                       .AsSingleRow<AllSupportedTypes>();

  EXPECT_EQ(row_to_insert.uint8, row.uint8);
  EXPECT_EQ(row_to_insert.int8, row.int8);
  EXPECT_EQ(row_to_insert.uint16, row.uint16);
  EXPECT_EQ(row_to_insert.int16, row.int16);
  EXPECT_EQ(row_to_insert.uint32, row.uint32);
  EXPECT_EQ(row_to_insert.int32, row.int32);
  EXPECT_EQ(row_to_insert.uint64, row.uint64);
  EXPECT_EQ(row_to_insert.int64, row.int64);
  EXPECT_FLOAT_EQ(row_to_insert.float_t, row.float_t);
  EXPECT_DOUBLE_EQ(row_to_insert.double_t, row.double_t);
  EXPECT_EQ(row_to_insert.string_t, row.string_t);
  EXPECT_EQ(ToMariaDBPrecision(row_to_insert.datetime_6_t), row.datetime_6_t);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
