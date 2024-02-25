#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <userver/cache/lru_map.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings_fwd.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/tracing/span.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pg_connection_wrapper.hpp>
#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/postgres/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class ConnectionImpl {
 public:
  struct PreparedStatementInfo {
    Connection::StatementId id{};
    std::string statement;
    std::string statement_name;
    ResultSet description{nullptr};
  };

  ConnectionImpl(engine::TaskProcessor& bg_task_processor,
                 concurrent::BackgroundTaskStorageCore& bg_task_storage,
                 uint32_t id, ConnectionSettings settings,
                 const DefaultCommandControls& default_cmd_ctls,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 const error_injection::Settings& ei_settings,
                 engine::SemaphoreLock&& size_lock);

  void AsyncConnect(const Dsn& dsn, engine::Deadline deadline);
  void Close();

  int GetServerVersion() const;
  bool IsInAbortedPipeline() const;
  bool IsInRecovery() const;
  bool IsReadOnly() const;
  void RefreshReplicaState(engine::Deadline deadline);

  ConnectionState GetConnectionState() const;
  bool IsConnected() const;
  bool IsIdle() const;
  bool IsInTransaction() const;
  bool IsPipelineActive() const;
  bool ArePreparedStatementsEnabled() const;
  bool IsBroken() const;
  bool IsExpired() const;
  ConnectionSettings const& GetSettings() const;

  CommandControl GetDefaultCommandControl() const;
  void UpdateDefaultCommandControl();

  const OptionalCommandControl& GetTransactionCommandControl() const;
  OptionalCommandControl GetNamedQueryCommandControl(
      const std::optional<Query::Name>& query_name) const;

  Connection::Statistics GetStatsAndReset();

  ResultSet ExecuteCommand(const Query& query,
                           const detail::QueryParameters& params,
                           OptionalCommandControl statement_cmd_ctl);

  const PreparedStatementInfo& PrepareStatement(
      const Query& query, const detail::QueryParameters& params,
      TimeoutDuration timeout);
  void AddIntoPipeline(CommandControl cc,
                       const std::string& prepared_statement_name,
                       const detail::QueryParameters& params,
                       const ResultSet& description, tracing::ScopeTime& scope);
  std::vector<ResultSet> GatherPipeline(
      TimeoutDuration timeout, const std::vector<ResultSet>& descriptions);

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

  void Listen(std::string_view channel, OptionalCommandControl);
  void Unlisten(std::string_view channel, OptionalCommandControl);
  Notification WaitNotify(engine::Deadline deadline);

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
  using PreparedStatements =
      cache::LruMap<Connection::StatementId, PreparedStatementInfo>;

  struct ResetTransactionCommandControl;

  void CheckBusy() const;
  void CheckDeadlineReached(const engine::Deadline& deadline);
  tracing::Span MakeQuerySpan(const Query& query,
                              const CommandControl& cc) const;
  engine::Deadline MakeCurrentDeadline() const;

  void SetTransactionCommandControl(CommandControl cmd_ctl);

  TimeoutDuration ExecuteTimeout(OptionalCommandControl) const;
  TimeoutDuration CurrentExecuteTimeout() const;

  void SetConnectionStatementTimeout(TimeoutDuration timeout,
                                     engine::Deadline deadline);

  void SetStatementTimeout(TimeoutDuration timeout, engine::Deadline deadline);

  void SetStatementTimeout(OptionalCommandControl cmd_ctl);

  const PreparedStatementInfo& DoPrepareStatement(
      const std::string& statement, const detail::QueryParameters& params,
      engine::Deadline deadline, tracing::Span& span,
      tracing::ScopeTime& scope);
  void DiscardOldPreparedStatements(engine::Deadline deadline);
  void DiscardPreparedStatement(const PreparedStatementInfo& info,
                                engine::Deadline deadline);

  ResultSet ExecuteCommand(const Query& query, engine::Deadline deadline);

  ResultSet ExecuteCommand(const Query& query,
                           const detail::QueryParameters& params,
                           engine::Deadline deadline);

  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    engine::Deadline deadline);

  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    const QueryParameters& params,
                                    engine::Deadline deadline);

  void SendCommandNoPrepare(const Query& query, engine::Deadline deadline);

  void SendCommandNoPrepare(const Query& query, const QueryParameters& params,
                            engine::Deadline deadline);

  void SetParameter(std::string_view name, std::string_view value,
                    Connection::ParameterScope scope,
                    engine::Deadline deadline);

  void LoadUserTypes(engine::Deadline deadline);
  void FillBufferCategories(ResultSet& res);

  template <typename Counter>
  ResultSet WaitResult(const std::string& statement, engine::Deadline deadline,
                       TimeoutDuration network_timeout, Counter& counter,
                       tracing::Span& span, tracing::ScopeTime& scope,
                       const ResultSet* description_ptr);

  void Cancel();

  void ReportStatement(const std::string& name);

  bool IsOmitDescribeInExecuteEnabled() const;

  const std::string uuid_;
  Connection::Statistics stats_;
  PGConnectionWrapper conn_wrapper_;
  PreparedStatements prepared_;
  UserTypes db_types_;
  bool is_in_recovery_ = true;
  bool is_read_only_ = true;
  bool is_discard_prepared_pending_ = false;
  ConnectionSettings settings_;
  std::optional<std::chrono::steady_clock::time_point> expires_at_;

  CommandControl default_cmd_ctl_{{}, {}};
  DefaultCommandControls default_cmd_ctls_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  OptionalCommandControl transaction_cmd_ctl_;
  TimeoutDuration current_statement_timeout_{};
  const error_injection::Settings ei_settings_;

  std::unordered_set<std::string> statements_reported_;
  engine::Mutex statements_mutex_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
