#pragma once

#include <memory>
#include <optional>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>

#include <userver/storages/mysql/impl/bind_helper.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>
#include <userver/storages/mysql/options.hpp>
#include <userver/storages/mysql/query.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>
#include <userver/storages/mysql/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace tests {
class TestsHelper;
}

namespace settings {
struct MysqlSettings;
}

namespace infra::topology {
class TopologyBase;
}

class Cluster final {
 public:
  Cluster(clients::dns::Resolver& resolver,
          const settings::MysqlSettings& settings,
          const components::ComponentConfig& config);
  ~Cluster();

  Transaction Begin(ClusterHostType host_type) const;

  Transaction Begin(OptionalCommandControl command_control,
                    ClusterHostType host_type) const;

  template <typename... Args>
  StatementResultSet Select(ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  template <typename... Args>
  StatementResultSet Select(OptionalCommandControl command_control,
                            ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  template <typename T>
  void InsertOne(const Query& query, const T& row) const;

  template <typename T>
  void InsertOne(OptionalCommandControl command_control, const Query& query,
                 const T& row) const;

  template <typename Container>
  void InsertMany(const Query& query, const Container& rows,
                  bool throw_on_empty_insert = true) const;

  template <typename Container>
  void InsertMany(OptionalCommandControl command_control, const Query& query,
                  const Container& rows,
                  bool throw_on_empty_insert = true) const;

  template <typename MapTo, typename Container>
  void InsertManyMapped(const Query& query, const Container& rows,
                        bool throw_on_empty_insert = true) const;

  template <typename MapTo, typename Container>
  void InsertManyMapped(OptionalCommandControl command_control,
                        const Query& query, const Container& rows,
                        bool throw_on_empty_insert = true) const;

  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  template <typename... Args>
  StatementResultSet Execute(OptionalCommandControl command_control,
                             ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  template <typename Container>
  void ExecuteBulk(ClusterHostType host_type, const Query& query,
                   const Container& params,
                   bool throw_on_empty_params = true) const;

  template <typename Container>
  void ExecuteBulk(OptionalCommandControl command_control,
                   ClusterHostType host_type, const Query& query,
                   const Container& params,
                   bool throw_on_empty_params = true) const;

  template <typename MapTo, typename Container>
  void ExecuteBulkMapped(ClusterHostType host_type, const Query& query,
                         const Container& params,
                         bool throw_on_empty_params = true) const;

  template <typename MapTo, typename Container>
  void ExecuteBulkMapped(OptionalCommandControl command_control,
                         ClusterHostType host_type, const Query& query,
                         const Container& params,
                         bool throw_on_empty_params = true) const;

  void ExecuteCommand(ClusterHostType host_type, const Query& command) const;

  void ExecuteCommand(OptionalCommandControl command_control,
                      ClusterHostType host_type, const Query& command) const;

  template <typename T, typename... Args>
  CursorResultSet<T> GetCursor(ClusterHostType host_type,
                               std::size_t batch_size, const Query& query,
                               const Args&... args) const;

  template <typename T, typename... Args>
  CursorResultSet<T> GetCursor(OptionalCommandControl command_control,
                               ClusterHostType host_type,
                               std::size_t batch_size, const Query& query,
                               const Args&... args) const;

 private:
  static CommandControl GetDefaultCommandControl();

  StatementResultSet DoExecute(
      OptionalCommandControl command_control, ClusterHostType host_type,
      const Query& query, impl::io::ParamsBinderBase& params,
      std::optional<std::size_t> batch_size = std::nullopt) const;

  friend class tests::TestsHelper;
  std::string EscapeString(std::string_view source) const;

  std::unique_ptr<infra::topology::TopologyBase> topology_;
};

template <typename... Args>
StatementResultSet Cluster::Select(ClusterHostType host_type,
                                   const Query& query,
                                   const Args&... args) const {
  return Select(std::nullopt, host_type, query, args...);
}

template <typename... Args>
StatementResultSet Cluster::Select(OptionalCommandControl command_control,
                                   ClusterHostType host_type,
                                   const Query& query,
                                   const Args&... args) const {
  return Execute(command_control, host_type, query, args...);
}

template <typename T>
void Cluster::InsertOne(const Query& query, const T& row) const {
  return InsertOne(std::nullopt, query, row);
}

template <typename T>
void Cluster::InsertOne(OptionalCommandControl command_control,
                        const Query& query, const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  DoExecute(command_control, ClusterHostType::kMaster, query, params_binder);
}

template <typename Container>
void Cluster::InsertMany(const Query& query, const Container& rows,
                         bool throw_on_empty_insert) const {
  InsertMany(std::nullopt, query, rows, throw_on_empty_insert);
}

template <typename Container>
void Cluster::InsertMany(OptionalCommandControl command_control,
                         const Query& query, const Container& rows,
                         bool throw_on_empty_insert) const {
  ExecuteBulk(command_control, ClusterHostType::kMaster, query, rows,
              throw_on_empty_insert);
}

template <typename MapTo, typename Container>
void Cluster::InsertManyMapped(const Query& query, const Container& rows,
                               bool throw_on_empty_insert) const {
  InsertManyMapped<MapTo>(std::nullopt, query, rows, throw_on_empty_insert);
}

template <typename MapTo, typename Container>
void Cluster::InsertManyMapped(OptionalCommandControl command_control,
                               const Query& query, const Container& rows,
                               bool throw_on_empty_insert) const {
  ExecuteBulkMapped<MapTo>(command_control, ClusterHostType::kMaster, query,
                           rows, throw_on_empty_insert);
}

template <typename... Args>
StatementResultSet Cluster::Execute(ClusterHostType host_type,
                                    const Query& query,
                                    const Args&... args) const {
  return Execute(std::nullopt, host_type, query, args...);
}

template <typename... Args>
StatementResultSet Cluster::Execute(OptionalCommandControl command_control,
                                    ClusterHostType host_type,
                                    const Query& query,
                                    const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder);
}

template <typename Container>
void Cluster::ExecuteBulk(ClusterHostType host_type, const Query& query,
                          const Container& params,
                          bool throw_on_empty_params) const {
  return ExecuteBulk(std::nullopt, host_type, query, params,
                     throw_on_empty_params);
}

template <typename Container>
void Cluster::ExecuteBulk(OptionalCommandControl command_control,
                          ClusterHostType host_type, const Query& query,
                          const Container& params,
                          bool throw_on_empty_params) const {
  if (params.empty()) {
    if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }
  }

  auto params_binder = impl::BindHelper::BindContainerAsParams(params);

  DoExecute(command_control, host_type, query.GetStatement(), params_binder);
}

template <typename MapTo, typename Container>
void Cluster::ExecuteBulkMapped(ClusterHostType host_type, const Query& query,
                                const Container& params,
                                bool throw_on_empty_params) const {
  ExecuteBulkMapped<MapTo>(std::nullopt, host_type, query, params,
                           throw_on_empty_params);
}

template <typename MapTo, typename Container>
void Cluster::ExecuteBulkMapped(OptionalCommandControl command_control,
                                ClusterHostType host_type, const Query& query,
                                const Container& params,
                                bool throw_on_empty_params) const {
  if (params.empty()) {
    if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }
  }

  auto params_binder =
      impl::BindHelper::BindContainerAsParamsMapped<MapTo>(params);

  DoExecute(command_control, host_type, query.GetStatement(), params_binder);
}

template <typename T, typename... Args>
CursorResultSet<T> Cluster::GetCursor(ClusterHostType host_type,
                                      std::size_t batch_size,
                                      const Query& query,
                                      const Args&... args) const {
  return GetCursor<T>(std::nullopt, host_type, batch_size, query, args...);
}

template <typename T, typename... Args>
CursorResultSet<T> Cluster::GetCursor(OptionalCommandControl command_control,
                                      ClusterHostType host_type,
                                      std::size_t batch_size,
                                      const Query& query,
                                      const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return CursorResultSet<T>{
      DoExecute(command_control, host_type, query, params_binder, batch_size)};
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
