#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <engine/deadline.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <error_injection/settings_fwd.hpp>
#include <testsuite/postgres_control.hpp>
#include <tracing/span.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pg_connection_wrapper.hpp>
#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/query.hpp>
#include <storages/postgres/result_set.hpp>

namespace storages::postgres::detail {

class ConnectionImpl {
 public:
  ConnectionImpl(engine::TaskProcessor& bg_task_processor, uint32_t id,
                 ConnectionSettings settings,
                 const DefaultCommandControls& default_cmd_ctls,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 const error_injection::Settings& ei_settings,
                 Connection::SizeGuard&& size_guard);

  void AsyncConnect(const Dsn& dsn);
  void Close();

  int GetServerVersion() const;
  bool IsInRecovery() const;
  bool IsReadOnly() const;
  void RefreshReplicaState(engine::Deadline deadline);

  ConnectionState GetConnectionState() const;
  bool IsConnected() const;
  bool IsIdle() const;
  bool IsInTransaction() const;

  CommandControl GetDefaultCommandControl() const;
  void UpdateDefaultCommandControl();

  const OptionalCommandControl& GetTransactionCommandControl() const;
  OptionalCommandControl GetNamedQueryCommandControl(
      const std::optional<Query::Name>& query_name) const;

  Connection::Statistics GetStatsAndReset();

  ResultSet ExecuteCommand(const Query& query,
                           const detail::QueryParameters& params,
                           OptionalCommandControl statement_cmd_ctl);

  void Begin(const TransactionOptions& options,
             SteadyClock::time_point trx_start_time,
             OptionalCommandControl trx_cmd_ctl = {});
  void Commit();
  void Rollback();

  void Start(SteadyClock::time_point start_time);
  void Finish();

  Connection::StatementId PortalBind(const std::string& statement,
                                     const std::string& portal_name,
                                     const detail::QueryParameters& params,
                                     OptionalCommandControl statement_cmd_ctl);

  ResultSet PortalExecute(Connection::StatementId statement_id,
                          const std::string& portal_name, std::uint32_t n_rows,
                          OptionalCommandControl statement_cmd_ctl);

  void CancelAndCleanup(TimeoutDuration timeout);
  bool Cleanup(TimeoutDuration timeout);

  void SetParameter(std::string_view name, std::string_view value,
                    Connection::ParameterScope scope);

  const UserTypes& GetUserTypes() const;
  void LoadUserTypes();

  TimeoutDuration GetIdleDuration() const;
  TimeoutDuration GetStatementTimeout() const;

  void Ping();
  void MarkAsBroken();

 private:
  struct PreparedStatementInfo {
    Connection::StatementId id{};
    std::string statement;
    std::string statement_name;
    ResultSet description{nullptr};
  };

  using PreparedStatements =
      std::unordered_map<Connection::StatementId, PreparedStatementInfo>;

  struct ResetTransactionCommandControl;

  void CheckBusy() const;
  engine::Deadline MakeCurrentDeadline() const;

  void SetTransactionCommandControl(CommandControl cmd_ctl);

  TimeoutDuration CurrentExecuteTimeout() const;

  void SetConnectionStatementTimeout(TimeoutDuration timeout,
                                     engine::Deadline deadline);

  void SetStatementTimeout(TimeoutDuration timeout, engine::Deadline deadline);

  void SetStatementTimeout(OptionalCommandControl cmd_ctl);

  const PreparedStatementInfo& PrepareStatement(
      const std::string& statement, const detail::QueryParameters& params,
      engine::Deadline deadline, tracing::Span& span, ScopeTime& scope);
  void DiscardOldPreparedStatements(engine::Deadline deadline);

  ResultSet ExecuteCommand(const Query& query, engine::Deadline deadline);

  ResultSet ExecuteCommand(const Query& query,
                           const detail::QueryParameters& params,
                           engine::Deadline deadline);

  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    engine::Deadline deadline);

  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    const QueryParameters& params,
                                    engine::Deadline deadline);

  void SetParameter(std::string_view name, std::string_view value,
                    Connection::ParameterScope scope,
                    engine::Deadline deadline);

  void LoadUserTypes(engine::Deadline deadline);
  void FillBufferCategories(ResultSet& res);

  template <typename Counter>
  ResultSet WaitResult(const std::string& statement, engine::Deadline deadline,
                       TimeoutDuration network_timeout, Counter& counter,
                       tracing::Span& span, ScopeTime& scope,
                       const ResultSet* description_ptr);

  void Cancel();

  const std::string uuid_;
  Connection::Statistics stats_;
  PGConnectionWrapper conn_wrapper_;
  PreparedStatements prepared_;
  UserTypes db_types_;
  bool is_in_recovery_ = true;
  bool is_read_only_ = true;
  bool is_discard_prepared_pending_ = false;
  ConnectionSettings settings_;

  CommandControl default_cmd_ctl_{{}, {}};
  DefaultCommandControls default_cmd_ctls_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  OptionalCommandControl transaction_cmd_ctl_;
  TimeoutDuration current_statement_timeout_{};
  const error_injection::Settings ei_settings_;
};

}  // namespace storages::postgres::detail
