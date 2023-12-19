#pragma once

/// @file userver/storages/clickhouse/cluster.hpp
/// @brief @copybrief storages::clickhouse::Cluster

#include <atomic>
#include <string>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>

#include <userver/storages/clickhouse/fwd.hpp>
#include <userver/storages/clickhouse/impl/insertion_request.hpp>
#include <userver/storages/clickhouse/impl/pool.hpp>
#include <userver/storages/clickhouse/options.hpp>
#include <userver/storages/clickhouse/query.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class ExecutionResult;

namespace impl {
struct ClickhouseSettings;
}

/// @ingroup userver_clients
///
/// @brief Interface for executing queries on a cluster of ClickHouse servers.
///
/// Usually retrieved from components::ClickHouse component.
class Cluster final {
 public:
  /// Cluster constructor
  /// @param resolver asynchronous DNS resolver
  /// @param settings struct with settings fields:
  /// endpoints - list of endpoints (host + port)
  /// auth_settings - authentication settings (user, password, database)
  /// @param config components::ClickHouse component config
  Cluster(clients::dns::Resolver& resolver,
          const impl::ClickhouseSettings& settings,
          const components::ComponentConfig& config);
  /// Cluster destructor
  ~Cluster();

  Cluster(const Cluster&) = delete;

  /// @brief Execute a statement at some host of the cluster
  /// with args as query parameters.
  template <typename... Args>
  ExecutionResult Execute(const Query& query, const Args&... args) const;

  /// @brief Execute a statement with specified command control settings
  /// at some host of the cluster with args as query parameters.
  template <typename... Args>
  ExecutionResult Execute(OptionalCommandControl, const Query& query,
                          const Args&... args) const;

  /// @brief Insert data at some host of the cluster;
  /// `T` is expected to be a struct of vectors of same length.
  /// @param table_name table to insert into
  /// @param column_names names of columns of the table
  /// @param data data to insert
  /// See @ref clickhouse_io for better understanding of T's requirements.
  template <typename T>
  void Insert(const std::string& table_name,
              const std::vector<std::string_view>& column_names,
              const T& data) const;

  /// @brief Insert data with specified command control settings
  /// at some host of the cluster;
  /// `T` is expected to be a struct of vectors of same length.
  /// @param table_name table to insert into
  /// @param column_names names of columns of the table
  /// @param data data to insert
  /// See @ref clickhouse_io for better understanding of T's requirements.
  template <typename T>
  void Insert(OptionalCommandControl, const std::string& table_name,
              const std::vector<std::string_view>& column_names,
              const T& data) const;

  /// @brief Insert data at some host of the cluster;
  /// `Container` is expected to be an iterable of clickhouse-mapped type.
  /// @param table_name table to insert into
  /// @param column_names names of columns of the table
  /// @param data data to insert
  /// See @ref clickhouse_io for better understanding of
  /// `Container::value_type`'s requirements.
  /// @note This version of insert is less performant than `Insert` (it makes 2
  /// copies of data instead of just 1 copy) due to implementation details, so
  /// consider using less convenient but more performant analogue if performance
  /// is a concern.
  template <typename Container>
  void InsertRows(const std::string& table_name,
                  const std::vector<std::string_view>& column_names,
                  const Container& data) const;

  /// @brief Insert data with specified command control settings
  /// at some host of the cluster;
  /// `Container` is expected to be an iterable of clickhouse-mapped type.
  /// @param table_name table to insert into
  /// @param column_names names of columns of the table
  /// @param data data to insert
  /// See @ref clickhouse_io for better understanding of
  /// `Container::value_type`'s requirements.
  /// @note This version of insert is less performant than `Insert` (it makes 2
  /// copies of data instead of just 1 copy) due to implementation details, so
  /// consider using less convenient but more performant analogue if performance
  /// is a concern.
  template <typename Container>
  void InsertRows(OptionalCommandControl, const std::string& table_name,
                  const std::vector<std::string_view>& column_names,
                  const Container& data) const;

  /// Write cluster statistics
  void WriteStatistics(
      USERVER_NAMESPACE::utils::statistics::Writer& writer) const;

  /// Exception that is thrown if all specified endpoints are unavailable
  class NoAvailablePoolError : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

 private:
  void DoInsert(OptionalCommandControl,
                const impl::InsertionRequest& request) const;

  ExecutionResult DoExecute(OptionalCommandControl, const Query& query) const;

  const impl::Pool& GetPool() const;

  std::vector<impl::Pool> pools_;
  mutable std::atomic<std::size_t> current_pool_ind_{0};
};

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
  const auto request =
      impl::InsertionRequest::Create(table_name, column_names, data);

  DoInsert(optional_cc, request);
}

template <typename Container>
void Cluster::InsertRows(const std::string& table_name,
                         const std::vector<std::string_view>& column_names,
                         const Container& data) const {
  InsertRows(OptionalCommandControl{}, table_name, column_names, data);
}

template <typename Container>
void Cluster::InsertRows(OptionalCommandControl optional_cc,
                         const std::string& table_name,
                         const std::vector<std::string_view>& column_names,
                         const Container& data) const {
  if (data.empty()) return;

  const auto request =
      impl::InsertionRequest::CreateFromRows(table_name, column_names, data);

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
