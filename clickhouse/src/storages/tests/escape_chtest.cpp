#include <userver/utest/utest.hpp>

#include <string>
#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

namespace io = storages::clickhouse::io;

struct DataWithValues final {
  std::vector<std::string> strings;
  std::vector<uint64_t> values;
};

using clock = std::chrono::system_clock;

struct DataWithDatetime final {
  std::vector<clock::time_point> datetime;
  std::vector<clock::time_point> datetime_milli;
  std::vector<clock::time_point> datetime_micro;
  std::vector<clock::time_point> datetime_nano;
};

struct DataWithOptValue final {
  std::string data;
  std::optional<uint64_t> opt_value;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithValues> {
  using mapped_type = std::tuple<columns::StringColumn, columns::UInt64Column>;
};

template <>
struct CppToClickhouse<DataWithDatetime> {
  using mapped_type =
      std::tuple<columns::DateTimeColumn, columns::DateTime64ColumnMilli,
                 columns::DateTime64ColumnMicro, columns::DateTime64ColumnNano>;
};

template <>
struct CppToClickhouse<DataWithOptValue> {
  using mapped_type =
      std::tuple<columns::StringColumn,
                 columns::NullableColumn<columns::UInt64Column>>;
};

}  // namespace storages::clickhouse::io

UTEST(ExecuteWithArgs, Basic) {
  ClusterWrapper cluster{};
  const storages::clickhouse::Query q{
      "SELECT {}, * FROM system.numbers limit {}"};

  const auto result = cluster->Execute(q, "we", 5).As<DataWithValues>();
  EXPECT_EQ(result.values.size(), 5);

  EXPECT_EQ(result.strings.size(), 5);
  EXPECT_EQ(result.strings.front(), "we");
}

UTEST(ExecuteWithArgs, DatesArgs) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(date DateTime, milli DateTime64(3), "
      "micro DateTime64(6), nano DateTime64(9))");

  const auto now = clock::now();
  const DataWithDatetime insert_data{{now}, {now}, {now}, {now}};
  cluster->Insert("tmp_table", {"date", "milli", "micro", "nano"}, insert_data);

  const storages::clickhouse::Query q{
      "SELECT date, milli, micro, nano FROM tmp_table WHERE "
      "date <= {} AND milli <= {} AND micro <= {} AND nano <= {}"};
  const auto res =
      cluster
          ->Execute(q, now, io::DateTime64Milli{now}, io::DateTime64Micro{now},
                    io::DateTime64Nano{now})
          .As<DataWithDatetime>();
  ASSERT_EQ(res.datetime.size(), 1);
}

UTEST(ExecuteWithArgs, MaliciousString) {
  ClusterWrapper cluster{};

  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS users "
      "(login String, password String)");
  cluster->Execute(
      "INSERT INTO users(login, password) VALUES "
      "('itrofimow', 'not_my_password')");

  const std::string malicious{"incorrect_password' OR login='itrofimow"};
  const storages::clickhouse::Query q{
      "SELECT login FROM users WHERE password = {}"};

  const auto res = cluster->Execute(q, malicious);
  EXPECT_EQ(res.GetRowsCount(), 0);
}

UTEST(ExecuteWithArgs, MultilineQueryWithComments) {
  ClusterWrapper cluster{};

  const storages::clickhouse::Query q{R"(
WITH
    // first we generate some numbers
    a AS (
      SELECT number as num FROM numbers(0, {0})
    ),
    // then we generate some string with numbers
    b AS (
      SELECT {1} as str, c.number as num FROM numbers(0, {0}) c
    )
-- and then join them
SELECT b.str, a.num FROM a JOIN b ON a.num = b.num; -- because why not
/* note that multi-statements are not allowed by clickhouse! */
)"};
  const auto res = cluster->Execute(q, 5, "we").As<DataWithValues>();
  EXPECT_EQ(res.strings.size(), 5);
  EXPECT_EQ(res.strings.front().size(), 2);
}

UTEST(ExecuteWithArgs, InsertSelectNull) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS fruits "
      "(fruit String, price Nullable(UInt64))");
  cluster->Execute(
      "INSERT INTO fruits(fruit, price) VALUES "
      "('apple', 300), "
      "('mango', NULL)");

  std::optional<uint64_t> null_price;
  const storages::clickhouse::Query query{
      "SELECT fruit, price FROM fruits "
      "WHERE price is {0}"};
  const auto null_rows = cluster->Execute(query, null_price)
                             .AsContainer<std::vector<DataWithOptValue>>();

  EXPECT_EQ(null_rows.size(), 1);
  EXPECT_EQ(null_rows[0].data, "mango");
  EXPECT_EQ(null_rows[0].opt_value, std::nullopt);
}

UTEST(ExecuteWithArgs, InsertSelectNotNull) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS fruits "
      "(fruit String, price Nullable(UInt64))");
  cluster->Execute(
      "INSERT INTO fruits(fruit, price) VALUES "
      "('apple', 300), "
      "('mango', NULL)");

  std::optional<uint64_t> price = 300;
  const storages::clickhouse::Query query{
      "SELECT fruit, price FROM fruits "
      "WHERE price = {0}"};
  const auto not_null_rows = cluster->Execute(query, price)
                                 .AsContainer<std::vector<DataWithOptValue>>();

  EXPECT_EQ(not_null_rows.size(), 1);
  EXPECT_EQ(not_null_rows[0].data, "apple");
  EXPECT_EQ(not_null_rows[0].opt_value.value(), 300);
}

USERVER_NAMESPACE_END
