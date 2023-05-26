#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/utils/statistics/min_max_avg.hpp>
#include <userver/utils/strong_typedef.hpp>
#include <utils/size_guard.hpp>

#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

enum class ConnectionState {
  kOffline,     //!< Not connected
  kIdle,        //!< Connected, not in transaction
  kTranIdle,    //!< In a valid transaction block, idle
  kTranActive,  //!< In a transaction, processing a SQL statement
  kTranError    //!< In a failed transaction block, idle
};

namespace detail {

class ConnectionImpl;

/// @brief PostreSQL connection class
/// Handles connecting to Postgres, sending commands, processing command results
/// and closing Postgres connection.
/// Responsible for all asynchronous operations
class Connection {
 public:
  enum class ParameterScope {
    kSession,     //!< Parameter is set for the duration of the whole session
    kTransaction  //!< Parameter will be in effect until the transaction is
                  //!< finished
  };

  /// Strong typedef for IDs assigned to prepared statements
  using StatementId =
      USERVER_NAMESPACE::utils::StrongTypedef<struct StatementIdTag,
                                              std::size_t>;

  /// @brief Statistics storage
  /// @note Should be reset after every transaction execution
  struct Statistics {
    using SmallCounter = uint8_t;
    using Counter = uint16_t;
    using CurrentValue = uint32_t;

    constexpr Statistics() noexcept
        : trx_total{0}, commit_total{0}, rollback_total{0}, out_of_trx{0} {}

    /// Number of transactions started
    // NOLINTNEXTLINE(modernize-use-default-member-init)
    SmallCounter trx_total : 1;
    /// Number of transactions committed
    // NOLINTNEXTLINE(modernize-use-default-member-init)
    SmallCounter commit_total : 1;
    /// Number of transactions rolled back
    // NOLINTNEXTLINE(modernize-use-default-member-init)
    SmallCounter rollback_total : 1;
    /// Number of out-of-transaction executions
    // NOLINTNEXTLINE(modernize-use-default-member-init)
    SmallCounter out_of_trx : 1;
    /// Number of parsed queries
    Counter parse_total{0};
    /// Number of query executions (calls to `Execute`)
    Counter execute_total{0};
    /// Total number of replies
    Counter reply_total{0};
    /// Number of portal bind operations
    Counter portal_bind_total{0};
    /// Error during query execution
    Counter error_execute_total{0};
    /// Timeout while executing
    Counter execute_timeout{0};
    /// Number of duplicate prepared statements errors,
    /// probably caused by timeout while preparing
    Counter duplicate_prepared_statements{0};

    /// Current number of prepared statements
    CurrentValue prepared_statements_current{0};

    /// Transaction initiation time (includes wait in pool)
    SteadyClock::time_point trx_start_time;
    /// Actual work start time (doesn't include pool wait time)
    SteadyClock::time_point work_start_time;
    /// Transaction end time (user called commit/rollback/finish)
    SteadyClock::time_point trx_end_time;
    /// Time of last statement executed, to calculate times between statement
    /// processing finish and user letting go of the connection.
    SteadyClock::time_point last_execute_finish;
    /// Sum of all query durations
    SteadyClock::duration sum_query_duration{0};
  };

  using SizeGuard =
      USERVER_NAMESPACE::utils::SizeGuard<std::shared_ptr<std::atomic<size_t>>>;

  Connection(const Connection&) = delete;
  Connection(Connection&&) = delete;
  ~Connection();

  // clang-format off
  /// Connect to database using DSN
  ///
  /// Will suspend current coroutine
  ///
  /// @param dsn DSN, @see https://www.postgresql.org/docs/12/static/libpq-connect.html#LIBPQ-CONNSTRING
  /// @param bg_task_processor task processor for blocking operations
  /// @param id host-wide unique id for connection identification in logs
  /// @param settings the connection settings
  /// @param default_cmd_ctls default parameters for operations
  /// @param testsuite_pg_ctl operation parameters customizer for testsuite
  /// @param ei_settings error injection settings
  /// @param size_guard structure to track the size of owning connection pool
  /// @throws ConnectionFailed, ConnectionTimeoutError
  // clang-format on
  static std::unique_ptr<Connection> Connect(
      const Dsn& dsn, clients::dns::Resolver* resolver,
      engine::TaskProcessor& bg_task_processor,
      concurrent::BackgroundTaskStorageCore& bg_task_storage, uint32_t id,
      ConnectionSettings settings,
      const DefaultCommandControls& default_cmd_ctls,
      const testsuite::PostgresControl& testsuite_pg_ctl,
      const error_injection::Settings& ei_settings,
      engine::SemaphoreLock&& size_lock = engine::SemaphoreLock{});

  /// Close the connection
  /// TODO When called from another thread/coroutine will wait for current
  /// transaction to finish.
  void Close();

  bool IsInRecovery() const;
  bool IsReadOnly() const;
  void RefreshReplicaState(engine::Deadline) const;
  ConnectionSettings const& GetSettings() const;

  /// Get current connection state
  ConnectionState GetState() const;
  /// Check if the connection is active
  bool IsConnected() const;
  /// Check if the connection is currently idle (IsConnected &&
  /// !IsInTransaction)
  bool IsIdle() const;
  /// Check if the connection is in unusable state
  bool IsBroken() const;

  /// The result is formed by multiplying the server's major version number by
  /// 10000 and adding the minor version number. -- docs
  /// Returns 0 if version cannot be determined.
  int GetServerVersion() const;

  /// Check if connection is currently in transaction
  bool IsInTransaction() const;

  CommandControl GetDefaultCommandControl() const;
  void UpdateDefaultCommandControl();

  /// Get currently accumulated statistics and reset counters
  /// @note May only be called when connection is not in transaction
  Statistics GetStatsAndReset();

  //@{
  /// Begin a transaction in Postgres with specific start time point
  /// Suspends coroutine for execution
  /// @throws AlreadyInTransaction
  void Begin(const TransactionOptions& options,
             SteadyClock::time_point trx_start_time,
             OptionalCommandControl trx_cmd_ctl = {});
  /// Commit current transaction
  /// Suspends coroutine for execution
  /// @throws NotInTransaction
  void Commit();
  /// Rollback current transaction
  /// Suspends coroutine for execution
  /// @throws NotInTransaction
  void Rollback();

  /// Mark start time of non-transaction execution, for stats
  void Start(SteadyClock::time_point start_time);
  /// Mark non-transaction execution finished, for stats
  void Finish();
  //@}

  //@{
  /** @name Command sending interface */
  ResultSet Execute(const Query& query, const detail::QueryParameters& = {},
                    OptionalCommandControl statement_cmd_ctl = {});

  template <typename... T>
  ResultSet Execute(const Query& query, const T&... args) {
    detail::StaticQueryParameters<sizeof...(args)> params;
    params.Write(GetUserTypes(), args...);
    return Execute(query, detail::QueryParameters{params});
  }

  template <typename... T>
  ResultSet Execute(CommandControl statement_cmd_ctl, const Query& query,
                    const T&... args) {
    detail::StaticQueryParameters<sizeof...(args)> params;
    params.Write(GetUserTypes(), args...);
    return Execute(query, params, OptionalCommandControl{statement_cmd_ctl});
  }

  ResultSet Execute(const Query& query, const ParameterStore& store);

  ResultSet Execute(CommandControl statement_cmd_ctl, const Query& query,
                    const ParameterStore& store);

  StatementId PortalBind(const std::string& statement,
                         const std::string& portal_name,
                         const detail::QueryParameters& params,
                         OptionalCommandControl);
  ResultSet PortalExecute(StatementId, const std::string& portal_name,
                          std::uint32_t n_rows, OptionalCommandControl);

  /// Send cancel to the database backend
  /// Try to return connection to idle state discarding all results.
  /// If there is a transaction in progress - roll it back.
  /// For usage in connection pools.
  /// Will do nothing if connection failed, it's responsibility of the pool
  /// to destroy the connection.
  void CancelAndCleanup(TimeoutDuration timeout);

  /// Wait while database connection is busy
  /// For usage in transaction pools, before an attempt to cancel
  /// If the connection is still busy, return false
  /// If the connection is in TranActive state return false
  /// If the connection is in TranIdle or TranError - rollback transaction and
  /// return true
  bool Cleanup(TimeoutDuration timeout);

  /// @brief Set session parameter
  /// Parameters documentation
  /// https://www.postgresql.org/docs/current/sql-set.html
  void SetParameter(const std::string& param, const std::string& value,
                    ParameterScope scope);

  /// @brief Reload user types after creating a type
  void ReloadUserTypes();
  const UserTypes& GetUserTypes() const;
  //@}

  /// Get duration since last network operation
  TimeoutDuration GetIdleDuration() const;
  /// Ping the connection.
  /// The function will do a query roundtrip to the database
  void Ping();

  void MarkAsBroken();

  OptionalCommandControl GetQueryCmdCtl(
      const std::optional<Query::Name>& query_name) const;

  /// Used in tests.
  const OptionalCommandControl& GetTransactionCommandControl() const;
  TimeoutDuration GetStatementTimeout() const;

 private:
  Connection();

  std::unique_ptr<ConnectionImpl> pimpl_;
};

}  // namespace detail
}  // namespace storages::postgres

USERVER_NAMESPACE_END
