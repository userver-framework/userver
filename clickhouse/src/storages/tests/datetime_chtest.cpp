#include <userver/utest/utest.hpp>

#include <chrono>
#include <vector>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithDatetime final {
  std::vector<std::chrono::system_clock::time_point> dates;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithDatetime> {
  using mapped_type = std::tuple<columns::DateTimeColumn>;
};

}  // namespace storages::clickhouse::io

UTEST(DateTime, InsertSelect) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value DateTime)");

  const auto now = std::chrono::system_clock::now();

  const DataWithDatetime insert_data{{now}};
  cluster->Insert("tmp_table", {"value"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT value FROM tmp_table").As<DataWithDatetime>();
  EXPECT_EQ(select_data.dates.size(), 1);
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                select_data.dates.front().time_since_epoch()),
            std::chrono::duration_cast<std::chrono::seconds>(
                insert_data.dates.front().time_since_epoch()));
}

struct TimepointData final {
  std::vector<std::chrono::system_clock::time_point> seconds;
  std::vector<std::chrono::system_clock::time_point> milli;
  std::vector<std::chrono::system_clock::time_point> micro;
  std::vector<std::chrono::system_clock::time_point> nano;
};

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<TimepointData> {
  using mapped_type =
      std::tuple<columns::DateTimeColumn, columns::DateTime64ColumnMilli,
                 columns::DateTime64ColumnMicro, columns::DateTime64ColumnNano>;
};

}  // namespace storages::clickhouse::io

UTEST(Datetime64, Precision) {
  ClusterWrapper cluster;
  cluster->Execute(
      "CREATE TEMPORARY TABLE test_tp"
      "(sec DateTime, milli DateTime64(3), "
      "micro DateTime64(6), nano DateTime64(9))");
  const auto now = std::chrono::system_clock::now();

  const TimepointData insert_data{{now}, {now}, {now}, {now}};
  cluster->Insert("test_tp", {"sec", "milli", "micro", "nano"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT sec, milli, micro, nano FROM test_tp")
          .As<TimepointData>();

  EXPECT_EQ(
      select_data.seconds.front().time_since_epoch(),
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()));
  EXPECT_EQ(select_data.milli.front().time_since_epoch(),
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()));
  EXPECT_EQ(select_data.micro.front().time_since_epoch(),
            std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()));
  EXPECT_EQ(select_data.nano.front().time_since_epoch(),
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()));
}

USERVER_NAMESPACE_END
