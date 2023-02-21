#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <storages/clickhouse/impl/connection_ptr.hpp>

#include "utils_test.hpp"

namespace {

struct DummyData {
  std::vector<std::string> data;
};

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DummyData> {
  using mapped_type = std::tuple<columns::StringColumn>;
};

}  // namespace storages::clickhouse::io

UTEST(Metrics, Basic) {
  ClusterWrapper cluster{};

  const DummyData data{{"str"}};

  // successful query
  cluster->Execute("CREATE TEMPORARY TABLE IF NOT EXISTS tmp(value String)");
  // successful insert
  cluster->Insert("tmp", {"value"}, data);

  const auto snapshot = cluster.GetStatistics("clickhouse.connections");
  EXPECT_EQ(

      snapshot.SingleMetric("active").AsInt(), 1);

  // unsuccessful query
  EXPECT_ANY_THROW(cluster->Execute("invalid_query_format"));
  // unsuccessful insert
  EXPECT_ANY_THROW(cluster->Insert("nonexistent", {"value"}, data));

  const auto connection_stats = cluster.GetStatistics("clickhouse.connections");
  const auto queries_stats = cluster.GetStatistics("clickhouse.queries");

  const auto insert_stats = cluster.GetStatistics("clickhouse.inserts");

  EXPECT_EQ(connection_stats.SingleMetric("closed").AsInt(), 2);

  EXPECT_EQ(queries_stats.SingleMetric("total").AsInt(), 2);
  EXPECT_EQ(queries_stats.SingleMetric("error").AsInt(), 1);

  EXPECT_EQ(insert_stats.SingleMetric("total").AsInt(), 2);
  EXPECT_EQ(insert_stats.SingleMetric("error").AsInt(), 1);
}

UTEST(Metrics, ActiveConnections) {
  PoolWrapper pool{};

  std::vector<storages::clickhouse::impl::ConnectionPtr> connections;
  for (size_t i = 0; i < 3; ++i) {
    connections.emplace_back(pool->Acquire());
  }

  EXPECT_EQ(pool->GetStatistics().connections.busy, 3);
  EXPECT_EQ(pool->GetStatistics().connections.active, 3);
  connections.clear();
  EXPECT_EQ(pool->GetStatistics().connections.busy, 0);
  EXPECT_EQ(pool->GetStatistics().connections.active, 3);
}

USERVER_NAMESPACE_END
