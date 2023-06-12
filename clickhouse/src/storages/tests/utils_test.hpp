#pragma once

#include <userver/utest/utest.hpp>

#include <storages/clickhouse/impl/pool_impl.hpp>
#include <storages/clickhouse/impl/settings.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

std::uint32_t GetClickhousePort();

class ClusterWrapper final {
 public:
  ClusterWrapper(
      bool use_compression = false,
      const std::vector<storages::clickhouse::impl::EndpointSettings>&
          endpoints = {{"localhost", GetClickhousePort()}});

  storages::clickhouse::Cluster* operator->();
  storages::clickhouse::Cluster& operator*();

  utils::statistics::Snapshot GetStatistics(
      std::string prefix,
      std::vector<utils::statistics::Label> require_labels = {});

 private:
  clients::dns::Resolver resolver_;
  USERVER_NAMESPACE::utils::statistics::Storage statistics_storage_;
  storages::clickhouse::Cluster cluster_;
  utils::statistics::Entry stats_holder_;
};

class PoolWrapper final {
 public:
  PoolWrapper();

  storages::clickhouse::impl::PoolImpl* operator->();
  storages::clickhouse::impl::PoolImpl& operator*();

 private:
  clients::dns::Resolver resolver_;
  std::shared_ptr<storages::clickhouse::impl::PoolImpl> pool_;
};

namespace storages::clickhouse::io {

class IteratorsTester final {
 public:
  static std::optional<std::string> GetCurrentValue(
      columns::ColumnIterator<columns::StringColumn>& iterator);

  template <typename T>
  static auto& GetCurrentIteratorsTuple(
      typename io::RowsMapper<T>::Iterator& iterator) {
    return iterator.iterators_;
  }

  template <typename T>
  static constexpr bool kCanMoveFromIterators =
      io::RowsMapper<T>::kCanMoveFromIterators;
};

}  // namespace storages::clickhouse::io

namespace storages::clickhouse {

class QueryTester final {
 public:
  template <typename... Args>
  static Query WithArgs(const Query& query, const Args&... args) {
    return query.WithArgs(args...);
  }
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
