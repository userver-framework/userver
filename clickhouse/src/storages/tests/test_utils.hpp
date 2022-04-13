#pragma once

#include <userver/clients/dns/resolver.hpp>
#include <userver/storages/clickhouse/cluster.hpp>

USERVER_NAMESPACE_BEGIN

class ClusterWrapper final {
 public:
  ClusterWrapper(bool use_compression = false);

  storages::clickhouse::Cluster* operator->();
  storages::clickhouse::Cluster& operator*();

 private:
  clients::dns::Resolver resolver_;
  storages::clickhouse::Cluster cluster_;
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
