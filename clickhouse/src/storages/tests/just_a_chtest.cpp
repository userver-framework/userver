#include <userver/utest/utest.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "test_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct SomeData final {
  std::vector<uint64_t> vec_uint64;
  std::vector<std::string> vec_str;
  std::vector<uint64_t> vec_uint64_2;
  std::vector<std::chrono::system_clock::time_point> vec_timepoint;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<SomeData> {
  using mapped_type =
      std::tuple<columns::UInt64Column, columns::StringColumn,
                 columns::UInt64Column, columns::DateTime64ColumnNano>;
};

}  // namespace storages::clickhouse::io

UTEST(Query, Works) {
  ClusterWrapper cluster{};

  storages::clickhouse::Query q{
      "SELECT c.number, randomString(10), c.number as t, NOW64() "
      "FROM "
      "numbers(0, 100000) c "};
  auto res = cluster->Execute(q).As<SomeData>();
  EXPECT_EQ(res.vec_str.size(), 100000);

  const storages::clickhouse::CommandControl cc{std::chrono::milliseconds{150}};
  auto native_res = cluster->Execute(cc, q);
  EXPECT_EQ(native_res.GetColumnsCount(), 4);
  EXPECT_EQ(native_res.GetRowsCount(), 100000);
  res = std::move(native_res).As<SomeData>();
  EXPECT_EQ(res.vec_str.size(), 100000);
  EXPECT_EQ(res.vec_uint64.size(), 100000);
  EXPECT_EQ(res.vec_uint64_2.size(), 100000);
  EXPECT_EQ(res.vec_timepoint.size(), 100000);
}

UTEST(Stats, Works) {
  ClusterWrapper cluster{};

  cluster->Execute({"SELECT 1;"});

  auto stats = cluster->GetStatistics();
  auto localhost = stats["localhost"];
  EXPECT_EQ(localhost["created"].As<size_t>(), 1);
  EXPECT_EQ(localhost["closed"].As<size_t>(), 0);
  EXPECT_EQ(localhost["overload"].As<size_t>(), 0);
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

USERVER_NAMESPACE_END
