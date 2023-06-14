#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct SomeData final {
  std::vector<uint64_t> vec_uint64;
  std::vector<std::string> vec_str;
  std::vector<uint64_t> vec_uint64_2;
  std::vector<std::chrono::system_clock::time_point> vec_timepoint;
};

struct SomeDataRow final {
  uint64_t uint64;
  std::string str;
  uint64_t uint64_2;
  std::chrono::system_clock::time_point timepoint;

  bool operator==(const SomeDataRow& other) const {
    return uint64 == other.uint64 && str == other.str &&
           uint64_2 == other.uint64_2 && timepoint == other.timepoint;
  }
};

struct SomeDataRowWithSleep final {
  uint64_t uint64{};
  std::string str;
  uint8_t sleep_result{};

  bool operator==(const SomeDataRowWithSleep& other) const {
    return uint64 == other.uint64 && str == other.str &&
           sleep_result == other.sleep_result;
  }
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<SomeData> {
  using mapped_type =
      std::tuple<columns::UInt64Column, columns::StringColumn,
                 columns::UInt64Column, columns::DateTime64ColumnNano>;
};

template <>
struct CppToClickhouse<SomeDataRow> {
  using mapped_type =
      std::tuple<columns::UInt64Column, columns::StringColumn,
                 columns::UInt64Column, columns::DateTime64ColumnNano>;
};

template <>
struct CppToClickhouse<SomeDataRowWithSleep> {
  using mapped_type = std::tuple<columns::UInt64Column, columns::StringColumn,
                                 columns::UInt8Column>;
};

}  // namespace storages::clickhouse::io

UTEST(Query, Works) {
  ClusterWrapper cluster{};

  const size_t expected_size = 10000;
  storages::clickhouse::Query q{
      "SELECT c.number, randomString(10), c.number as t, NOW64() "
      "FROM "
      "numbers(0, 10000) c "};
  auto res = cluster->Execute(q).As<SomeData>();
  EXPECT_EQ(res.vec_str.size(), expected_size);

  const storages::clickhouse::CommandControl cc{std::chrono::milliseconds{150}};
  auto native_res = cluster->Execute(cc, q);
  EXPECT_EQ(native_res.GetColumnsCount(), 4);
  EXPECT_EQ(native_res.GetRowsCount(), expected_size);
  res = std::move(native_res).As<SomeData>();
  EXPECT_EQ(res.vec_str.size(), expected_size);
  EXPECT_EQ(res.vec_uint64.size(), expected_size);
  EXPECT_EQ(res.vec_uint64_2.size(), expected_size);
  EXPECT_EQ(res.vec_timepoint.size(), expected_size);
}

UTEST(Compression, Works) {
  ClusterWrapper cluster{true};
  storages::clickhouse::Query q{
      "SELECT c.number, randomString(10), c.number as t, NOW64() "
      "FROM "
      "numbers(0, 10000) c "};
  auto res = cluster->Execute(q).As<SomeData>();
  EXPECT_EQ(res.vec_str.size(), 10000);
}

UTEST(Insert, Works) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table (id UInt64, value "
      "String, count UInt64, tp DateTime64(9))");

  const auto now = std::chrono::system_clock::now();

  SomeData data;
  data.vec_uint64.push_back(1);
  data.vec_str.emplace_back("asd");
  data.vec_uint64_2.push_back(2);
  data.vec_timepoint.push_back(now);
  cluster->Insert("tmp_table", {"id", "value", "count", "tp"}, data);

  auto result = cluster->Execute("SELECT id, value, count, tp FROM tmp_table")
                    .As<SomeData>();
  EXPECT_EQ(result.vec_uint64.size(), 1);
  EXPECT_EQ(result.vec_uint64.back(), 1);
  EXPECT_EQ(result.vec_str.back(), "asd");
  EXPECT_EQ(result.vec_uint64_2.back(), 2);
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::nanoseconds>(
                result.vec_timepoint.back().time_since_epoch())
                .count(),
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch())
                .count());
}

UTEST(Insert, AsRowsWorks) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table (id UInt64, value "
      "String, count UInt64, tp DateTime64(9))");

  const auto now = std::chrono::system_clock::now();
  std::vector<SomeDataRow> data{{1, "first", 2, now}, {3, "second", 4, now}};
  cluster->InsertRows("tmp_table", {"id", "value", "count", "tp"}, data);

  const auto result =
      cluster->Execute("SELECT id, value, count, tp FROM tmp_table ORDER BY id")
          .AsContainer<std::vector<SomeDataRow>>();
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], data[0]);
  EXPECT_EQ(result[1], data[1]);
}

UTEST(Query, AvoidUnexpectedCancellation) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table_with_sleep (id UInt64, "
      "value "
      "String, sleep_result UInt8)");

  std::vector<SomeDataRowWithSleep> data{{1, "first", 0}, {3, "second", 0}};
  cluster->InsertRows("tmp_table_with_sleep", {"id", "value", "sleep_result"},
                      data);

  // 2000ms to avoid flaps in CI, in perfect world ~300 should do
  const storages::clickhouse::CommandControl cc{
      std::chrono::milliseconds{2000}};
  const auto result = cluster
                          ->Execute(cc,
                                    "SELECT id, value, sleepEachRow(0.1) FROM "
                                    "tmp_table_with_sleep ORDER BY id")
                          .AsContainer<std::vector<SomeDataRowWithSleep>>();
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], data[0]);
  EXPECT_EQ(result[1], data[1]);
}

USERVER_NAMESPACE_END
