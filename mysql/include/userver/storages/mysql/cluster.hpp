#pragma once

/// @file userver/storages/mysql/cluster.hpp

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

/// @brief Client interface for a cluster of MySQL servers.
/// Usually retrieved from components::MySQL
class Cluster final {
 public:
  /// @brief Cluster constructor
  Cluster(clients::dns::Resolver& resolver,
          const settings::MysqlSettings& settings,
          const components::ComponentConfig& config);
  /// @brief Cluster destructor
  ~Cluster();

  /// @brief Begin a transaction with default deadline.
  ///
  /// @note The deadline is transaction-wide, not just for Begin query itself.
  ///
  /// @param host_type Host type on which to execute transaction.
  Transaction Begin(ClusterHostType host_type) const;

  /// @brief Being a transaction with specified CommandControl.
  ///
  /// @note The deadline is transaction-wide, not just for Begin query itself.
  ///
  /// @param host_type Host type on which to execute transaction.
  Transaction Begin(OptionalCommandControl command_control,
                    ClusterHostType host_type) const;

  /// @brief An alias for `Execute`.
  template <typename... Args>
  StatementResultSet Select(ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  /// @brief An alias for `Execute`.
  template <typename... Args>
  StatementResultSet Select(OptionalCommandControl command_control,
                            ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  /// @brief Insert single row into database with default deadline.
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// Basically an alias for Execute(query, AsArgs<T>(row)), where AsArgs is an
  /// imaginary function which passes fields of T as variadic params.
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename T>
  ExecutionResult InsertOne(const Query& query, const T& row) const;

  /// @brief Insert single row into database with provided CommandControl.
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// Basically an alias for Execute(command_control, ClusterHostType::kMaster,
  /// query, AsArgs<T>(row)), where AsArgs is an imaginary function which passes
  /// fields of T as variadic params.
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename T>
  ExecutionResult InsertOne(OptionalCommandControl command_control,
                            const Query& query, const T& row) const;

  /// @brief Inserts multiple rows into database with default deadline.
  /// `Container` is expected to be a std::Container of aggregates of
  /// supported types.
  ///
  /// Basically an alias for ExecuteBulk(ClusterHostType::kMaster, query, rows,
  /// throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  ExecutionResult InsertMany(const Query& query, const Container& rows,
                             bool throw_on_empty_insert = true) const;

  /// @brief Inserts multiple rows into database with provided CommandControl.
  /// `Container` is expected to be a std::Container of aggregates of
  /// supported types.
  ///
  /// Basically an alias for ExecuteBulk(command_control,
  /// ClusterHostType::kMaster, query, rows, throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  ExecutionResult InsertMany(OptionalCommandControl command_control,
                             const Query& query, const Container& rows,
                             bool throw_on_empty_insert = true) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Inserts multiple rows into database with default deadline, on the
  /// flight mapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or `storages::mysql::convert`.
  ///
  /// Basically an alias for ExecuteBulkMapped<MapTo>(ClusterHostType::kMaster,
  /// query, rows, throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  ExecutionResult InsertManyMapped(const Query& query, const Container& rows,
                                   bool throw_on_empty_insert = true) const;
  // clang-format on

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Inserts multiple rows into database with provided CommandControl, on the
  /// flight remapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// Basically an alias for ExecuteBulkMapped<MapTo>(command_control,
  /// ClusterHostType::kMaster, query, rows, throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  ExecutionResult InsertManyMapped(OptionalCommandControl command_control,
                                   const Query& query, const Container& rows,
                                   bool throw_on_empty_insert = true) const;
  // clang-format on

  /// @brief Executes a statement on a host of host_type with default deadline.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(OptionalCommandControl command_control,
                             ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  /// @brief Executes a statements on a host of host_type with default deadline.
  /// Fills placeholders of the statements with Container::value_type in a
  /// bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  StatementResultSet ExecuteBulk(ClusterHostType host_type, const Query& query,
                                 const Container& params,
                                 bool throw_on_empty_params = true) const;

  /// @brief Executes a statements on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statements with
  /// Container::value_type in a bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  StatementResultSet ExecuteBulk(OptionalCommandControl command_control,
                                 ClusterHostType host_type, const Query& query,
                                 const Container& params,
                                 bool throw_on_empty_params = true) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statements on a host of host_type with default deadline,
  /// on the flight remapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params,
                                       bool throw_on_empty_params = true) const;
  // clang-format on

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statements on a host of host_type with provided
  /// CommandControl, on the flight remapping from `Container::value_type`
  /// to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// @note Requires MariaDB 10.2+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(OptionalCommandControl command_control,
                                       ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params,
                                       bool throw_on_empty_params = true) const;
  // clang-format on

  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with default deadline.
  void ExecuteCommand(ClusterHostType host_type, const Query& command) const;

  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with provided CommandControl.
  void ExecuteCommand(OptionalCommandControl command_control,
                      ClusterHostType host_type, const Query& command) const;

  /// @brief Executes a statements with default deadline on a host of host_type,
  /// filling statements placeholders with `args...`, and returns a read-only
  /// cursor which fetches `batch_count` rows in each next fetch request.
  ///
  /// @note Deadline is processing-wide, not just for initial cursor creation.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename T, typename... Args>
  CursorResultSet<T> GetCursor(ClusterHostType host_type,
                               std::size_t batch_size, const Query& query,
                               const Args&... args) const;
  /// @brief Executes a statements with provided CommandControl on
  /// a host of host_type, filling statements placeholders with `args...`, and
  /// returns a read-only cursor which fetches `batch_count` rows in each next
  /// fetch request.
  ///
  /// @note Deadline is processing-wide, not just for initial cursor creation.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
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
ExecutionResult Cluster::InsertOne(const Query& query, const T& row) const {
  return InsertOne(std::nullopt, query, row);
}

template <typename T>
ExecutionResult Cluster::InsertOne(OptionalCommandControl command_control,
                                   const Query& query, const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  return DoExecute(command_control, ClusterHostType::kMaster, query,
                   params_binder)
      .AsExecutionResult();
}

template <typename Container>
ExecutionResult Cluster::InsertMany(const Query& query, const Container& rows,
                                    bool throw_on_empty_insert) const {
  return InsertMany(std::nullopt, query, rows, throw_on_empty_insert);
}

template <typename Container>
ExecutionResult Cluster::InsertMany(OptionalCommandControl command_control,
                                    const Query& query, const Container& rows,
                                    bool throw_on_empty_insert) const {
  return ExecuteBulk(command_control, ClusterHostType::kMaster, query, rows,
                     throw_on_empty_insert)
      .AsExecutionResult();
}

template <typename MapTo, typename Container>
ExecutionResult Cluster::InsertManyMapped(const Query& query,
                                          const Container& rows,
                                          bool throw_on_empty_insert) const {
  return InsertManyMapped<MapTo>(std::nullopt, query, rows,
                                 throw_on_empty_insert);
}

template <typename MapTo, typename Container>
ExecutionResult Cluster::InsertManyMapped(
    OptionalCommandControl command_control, const Query& query,
    const Container& rows, bool throw_on_empty_insert) const {
  return ExecuteBulkMapped<MapTo>(command_control, ClusterHostType::kMaster,
                                  query, rows, throw_on_empty_insert)
      .AsExecutionResult();
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
StatementResultSet Cluster::ExecuteBulk(ClusterHostType host_type,
                                        const Query& query,
                                        const Container& params,
                                        bool throw_on_empty_params) const {
  return ExecuteBulk(std::nullopt, host_type, query, params,
                     throw_on_empty_params);
}

template <typename Container>
StatementResultSet Cluster::ExecuteBulk(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Container& params,
    [[maybe_unused]] bool throw_on_empty_params) const {
  if (params.empty()) {
    // TODO : what do?
    throw std::runtime_error{"Empty params in bulk execution"};
    /*if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }*/
  }

  auto params_binder = impl::BindHelper::BindContainerAsParams(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder);
}

template <typename MapTo, typename Container>
StatementResultSet Cluster::ExecuteBulkMapped(
    ClusterHostType host_type, const Query& query, const Container& params,
    bool throw_on_empty_params) const {
  return ExecuteBulkMapped<MapTo>(std::nullopt, host_type, query, params,
                                  throw_on_empty_params);
}

template <typename MapTo, typename Container>
StatementResultSet Cluster::ExecuteBulkMapped(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Container& params,
    [[maybe_unused]] bool throw_on_empty_params) const {
  if (params.empty()) {
    // TODO : what do?
    throw std::runtime_error{"Empty params in bulk execution"};
    /*if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }*/
  }

  auto params_binder =
      impl::BindHelper::BindContainerAsParamsMapped<MapTo>(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder);
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
