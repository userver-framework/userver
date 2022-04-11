#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>

#include <userver/formats/json_fwd.hpp>
#include <userver/storages/clickhouse/insertion_request.hpp>
#include <userver/storages/clickhouse/options.hpp>
#include <userver/storages/clickhouse/pool.hpp>
#include <userver/storages/clickhouse/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class ExecutionResult;

struct ClickhouseSettings;

class Cluster final {
 public:
  Cluster(clients::dns::Resolver*, const ClickhouseSettings&,
          const components::ComponentConfig&);
  ~Cluster();

  Cluster(const Cluster&) = delete;

  template <typename... Args>
  ExecutionResult Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ExecutionResult Execute(OptionalCommandControl, const Query& query,
                          const Args&... args) const;

  template <typename T>
  void Insert(const std::string& table_name,
              const std::vector<std::string_view>& column_names,
              const T& data) const;

  template <typename T>
  void Insert(OptionalCommandControl, const std::string& table_name,
              const std::vector<std::string_view>& column_names,
              const T& data) const;

  formats::json::Value GetStatistics() const;

 private:
  ExecutionResult DoExecute(OptionalCommandControl, const Query& query) const;

  void DoInsert(OptionalCommandControl, const InsertionRequest& request) const;

  const Pool& GetPool() const;

  std::vector<Pool> pools_;
  mutable std::atomic<std::size_t> current_pool_ind_{0};
};

using ClusterPtr = std::shared_ptr<Cluster>;

template <typename T>
void Cluster::Insert(const std::string& table_name,
                     const std::vector<std::string_view>& column_names,
                     const T& data) const {
  Insert(OptionalCommandControl{}, table_name, column_names, data);
}

template <typename T>
void Cluster::Insert(OptionalCommandControl optional_cc,
                     const std::string& table_name,
                     const std::vector<std::string_view>& column_names,
                     const T& data) const {
  const auto request = InsertionRequest::Create(table_name, column_names, data);

  DoInsert(optional_cc, request);
}

template <typename... Args>
ExecutionResult Cluster::Execute(const Query& query,
                                 const Args&... args) const {
  return Execute(OptionalCommandControl{}, query, args...);
}

template <typename... Args>
ExecutionResult Cluster::Execute(OptionalCommandControl optional_cc,
                                 const Query& query,
                                 const Args&... args) const {
  const auto formatted_query = query.WithArgs(args...);
  return DoExecute(optional_cc, formatted_query);
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
