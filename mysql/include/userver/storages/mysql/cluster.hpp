#pragma once

/// @file userver/storages/mysql/cluster.hpp

#include <memory>
#include <optional>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/deadline.hpp>

#include <userver/storages/mysql/impl/bind_helper.hpp>

#include <userver/storages/mysql/client_interface.hpp>
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
class Cluster final : public IClientInterface {
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

  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with default deadline.
  ///
  /// This method is intended to be used for statements that cannot be prepared
  /// or as an escape hatch from typed parsing if you really need to, but such
  /// use is neither recommended nor optimized for.
  CommandResultSet ExecuteCommand(ClusterHostType host_type,
                                  const Query& command) const;

  /// @brief Executes a command on host of type host_type over plan-text
  /// protocol, with provided CommandControl.
  ///
  /// This method is intended to be used for statements that cannot be prepared
  /// or as an escape hatch from typed parsing if you really need to, but such
  /// use is neither recommended nor optimized for.
  CommandResultSet ExecuteCommand(OptionalCommandControl command_control,
                                  ClusterHostType host_type,
                                  const Query& command) const;

  /// @brief Executes a statements with default deadline on a host of host_type,
  /// filling statements placeholders with `args...`, and returns a read-only
  /// cursor which fetches `batch_count` rows in each next fetch request.
  /// See @ref userver_mysql_types for better understanding of `Args`
  /// requirements.
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
  /// See @ref userver_mysql_types for better understanding of `Args`
  /// requirements.
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
      std::optional<std::size_t> batch_size) const override;

  friend class tests::TestsHelper;
  std::string EscapeString(std::string_view source) const;

  std::unique_ptr<infra::topology::TopologyBase> topology_;
};

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
