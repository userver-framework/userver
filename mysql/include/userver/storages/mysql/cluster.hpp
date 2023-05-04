#pragma once

/// @file userver/storages/mysql/cluster.hpp
/// @copybrief @copybrief storages::mysql::Cluster

#include <memory>
#include <optional>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>
#include <userver/storages/mysql/command_result_set.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>
#include <userver/storages/mysql/impl/bind_helper.hpp>
#include <userver/storages/mysql/options.hpp>
#include <userver/storages/mysql/query.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>
#include <userver/storages/mysql/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace settings {
struct MysqlSettings;
}

namespace infra::topology {
class TopologyBase;
}

/// @ingroup userver_clients
///
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

  /// @brief Executes a statement on a host of host_type with default deadline.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Args` requirements.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  // clang-format off
  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of `Args`
  /// requirements.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  ///
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster Execute
  // clang-format on
  template <typename... Args>
  StatementResultSet Execute(OptionalCommandControl command_control,
                             ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  /// @brief Executes a statement on a host of host_type with default deadline
  ///
  /// Basically an alias for Execute(host_type, query, AsArgs<T>(row)),
  /// where AsArgs is an imaginary function which passes fields of T as
  /// variadic params. Handy for one-liner inserts.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `T` requirements.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename T>
  StatementResultSet ExecuteDecompose(ClusterHostType host_type,
                                      const Query& query, const T& row) const;

  // clang-format off
  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl.
  ///
  /// Basically an alias for Execute(command_control, host_type, query,
  /// AsArgs<T>(row)), where AsArgs is an imaginary function which passes
  /// fields of T as variadic params. Handy for one-liner inserts.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of `T` requirements.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  ///
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster ExecuteDecompose
  // clang-format on
  template <typename T>
  StatementResultSet ExecuteDecompose(OptionalCommandControl command_control,
                                      ClusterHostType host_type,
                                      const Query& query, const T& row) const;

  /// @brief Executes a statement on a host of host_type with default deadline.
  /// Fills placeholders of the statements with Container::value_type in a
  /// bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  template <typename Container>
  StatementResultSet ExecuteBulk(ClusterHostType host_type, const Query& query,
                                 const Container& params) const;

  // clang-format off
  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statements with
  /// Container::value_type in a bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster ExecuteBulk
  // clang-format on
  template <typename Container>
  StatementResultSet ExecuteBulk(OptionalCommandControl command_control,
                                 ClusterHostType host_type, const Query& query,
                                 const Container& params) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statement on a host of host_type with default deadline,
  /// on the flight remapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of `MapTo` requirements.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params) const;
  // clang-format on

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl, on the flight remapping from `Container::value_type`
  /// to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of `MapTo` requirements.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  ///
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster ExecuteBulkMapped
  // clang-format on
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(OptionalCommandControl command_control,
                                       ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params) const;

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

  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with default deadline.
  ///
  /// This method is intended to be used for statements that cannot be prepared
  /// or as an escape hatch from typed parsing if you really need to, but such
  /// use is neither recommended nor optimized for.
  CommandResultSet ExecuteCommand(ClusterHostType host_type,
                                  const Query& command) const;

  // clang-format off
  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with provided CommandControl.
  ///
  /// This method is intended to be used for statements that cannot be prepared
  /// or as an escape hatch from typed parsing if you really need to, but such
  /// use is neither recommended nor optimized for.
  ///
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster ExecuteCommand
  // clang-format on
  CommandResultSet ExecuteCommand(OptionalCommandControl command_control,
                                  ClusterHostType host_type,
                                  const Query& command) const;

  /// @brief Executes a statement with default deadline on a host of host_type,
  /// filling statements placeholders with `args...`, and returns a read-only
  /// cursor which fetches `batch_count` rows in each next fetch request.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Args` requirements.
  ///
  /// @note Deadline is processing-wide, not just for initial cursor creation.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename T, typename... Args>
  CursorResultSet<T> GetCursor(ClusterHostType host_type,
                               std::size_t batch_size, const Query& query,
                               const Args&... args) const;

  // clang-format off
  /// @brief Executes a statement with provided CommandControl on
  /// a host of host_type, filling statements placeholders with `args...`, and
  /// returns a read-only cursor which fetches `batch_count` rows in each next
  /// fetch request.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of `Args`
  /// requirements.
  ///
  /// @note Deadline is processing-wide, not just for initial cursor creation.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  ///
  /// @snippet storages/tests/unittests/cluster_mysqltest.cpp uMySQL usage sample - Cluster GetCursor
  // clang-format on
  template <typename T, typename... Args>
  CursorResultSet<T> GetCursor(OptionalCommandControl command_control,
                               ClusterHostType host_type,
                               std::size_t batch_size, const Query& query,
                               const Args&... args) const;

  /// Write cluster statistics
  void WriteStatistics(utils::statistics::Writer& writer) const;

 private:
  static CommandControl GetDefaultCommandControl();

  StatementResultSet DoExecute(OptionalCommandControl command_control,
                               ClusterHostType host_type, const Query& query,
                               impl::io::ParamsBinderBase& params,
                               std::optional<std::size_t> batch_size) const;

  std::unique_ptr<infra::topology::TopologyBase> topology_;
};

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
                   params_binder, std::nullopt);
}

template <typename T>
StatementResultSet Cluster::ExecuteDecompose(ClusterHostType host_type,
                                             const Query& query,
                                             const T& row) const {
  return ExecuteDecompose(std::nullopt, host_type, query, row);
}

template <typename T>
StatementResultSet Cluster::ExecuteDecompose(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  return DoExecute(command_control, host_type, query, params_binder,
                   std::nullopt);
}

template <typename Container>
StatementResultSet Cluster::ExecuteBulk(ClusterHostType host_type,
                                        const Query& query,
                                        const Container& params) const {
  return ExecuteBulk(std::nullopt, host_type, query, params);
}

template <typename Container>
StatementResultSet Cluster::ExecuteBulk(OptionalCommandControl command_control,
                                        ClusterHostType host_type,
                                        const Query& query,
                                        const Container& params) const {
  UINVARIANT(!params.empty(), "Empty params in bulk execution");

  auto params_binder = impl::BindHelper::BindContainerAsParams(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder, std::nullopt);
}

template <typename MapTo, typename Container>
StatementResultSet Cluster::ExecuteBulkMapped(ClusterHostType host_type,
                                              const Query& query,
                                              const Container& params) const {
  return ExecuteBulkMapped<MapTo>(std::nullopt, host_type, query, params);
}

template <typename MapTo, typename Container>
StatementResultSet Cluster::ExecuteBulkMapped(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Container& params) const {
  UINVARIANT(!params.empty(), "Empty params in bulk execution");

  auto params_binder =
      impl::BindHelper::BindContainerAsParamsMapped<MapTo>(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder, std::nullopt);
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
