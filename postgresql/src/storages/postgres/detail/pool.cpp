#include <storages/postgres/detail/pool.hpp>

#include <storages/postgres/deadline.hpp>
#include <storages/postgres/detail/cc_config.hpp>
#include <storages/postgres/detail/statement_timings_storage.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

USERVER_NAMESPACE::utils::impl::UserverExperiment kCcExperiment("pg-cc");

constexpr std::chrono::seconds kRecentErrorPeriod{15};

// Part of max_pool that can be cancelled at once
constexpr std::size_t kCancelRatio = 2;
constexpr std::chrono::seconds kCancelPeriod{1};

constexpr std::chrono::seconds kCleanupTimeout{1};

constexpr std::chrono::seconds kMaintainInterval{30};
constexpr std::chrono::seconds kMaxIdleDuration{15};
constexpr const char* kMaintainTaskName = "pg_maintain";

constexpr std::chrono::seconds kConnectingTimeout{2};
constexpr auto kPendingConnectsMax{1};

// Max idle connections that can be dropped in one run of maintenance task
constexpr auto kIdleDropLimit = 1;

// Practically unlimited number on concurrent establishing connections
constexpr auto kUnlimitedConnecting = std::numeric_limits<std::size_t>::max();

class Stopwatch {
 public:
  using Accumulator =
      USERVER_NAMESPACE::utils::statistics::RecentPeriod<Percentile, Percentile,
                                                         detail::SteadyClock>;
  explicit Stopwatch(Accumulator& acc)
      : accum_{acc}, start_{SteadyClock::now()} {}
  ~Stopwatch() {
    accum_.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            SteadyClock::now() - start_)
            .count());
  }

 private:
  Accumulator& accum_;
  SteadyClock::time_point start_;
};

}  // namespace

class ConnectionPool::EmplaceEnabler {};

ConnectionPool::ConnectionPool(
    EmplaceEnabler, Dsn dsn, clients::dns::Resolver* resolver,
    engine::TaskProcessor& bg_task_processor, const std::string& db_name,
    const PoolSettings& settings, const ConnectionSettings& conn_settings,
    const StatementMetricsSettings& statement_metrics_settings,
    const DefaultCommandControls& default_cmd_ctls,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings,
    const congestion_control::v2::LinearController::StaticConfig& cc_config,
    dynamic_config::Source config_source)
    : dsn_{std::move(dsn)},
      resolver_{resolver},
      db_name_{db_name},
      settings_{settings},
      conn_settings_{conn_settings},
      bg_task_processor_{bg_task_processor},
      queue_{settings.max_size},
      size_semaphore_{settings.max_size},
      connecting_semaphore_{settings.connecting_limit
                                ? settings.connecting_limit
                                : kUnlimitedConnecting},
      wait_count_{0},
      default_cmd_ctls_(default_cmd_ctls),
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_(std::move(ei_settings)),
      cancel_limit_{std::max(std::size_t{1}, settings.max_size / kCancelRatio),
                    {1, kCancelPeriod}},
      sts_{statement_metrics_settings},
      config_source_(config_source),
      cc_sensor_(*this),
      cc_limiter_(*this),
      cc_controller_("postgres" + db_name, cc_sensor_, cc_limiter_,
                     stats_.congestion_control, cc_config, config_source,
                     [](const dynamic_config::Snapshot& config) {
                       return config.Get<CcConfig>().config;
                     }) {
  if (kCcExperiment.IsEnabled()) {
    cc_controller_.Start();
  }
}

ConnectionPool::~ConnectionPool() {
  StopMaintainTask();
  StopConnectTasks();
  Clear();
}

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    Dsn dsn, clients::dns::Resolver* resolver,
    engine::TaskProcessor& bg_task_processor, const std::string& db_name,
    const InitMode& init_mode, const PoolSettings& pool_settings,
    const ConnectionSettings& conn_settings,
    const StatementMetricsSettings& statement_metrics_settings,
    const DefaultCommandControls& default_cmd_ctls,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings,
    const congestion_control::v2::LinearController::StaticConfig& cc_config,
    dynamic_config::Source config_source) {
  // FP?: pointer magic in boost.lockfree
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  auto impl = std::make_shared<ConnectionPool>(
      EmplaceEnabler{}, std::move(dsn), resolver, bg_task_processor, db_name,
      pool_settings, conn_settings, statement_metrics_settings,
      default_cmd_ctls, testsuite_pg_ctl, std::move(ei_settings), cc_config,
      config_source);
  // Init() uses shared_from_this for connections and cannot be called from
  // ctor
  impl->Init(init_mode);
  return impl;
}

void ConnectionPool::Init(InitMode mode) {
  if (dsn_.GetUnderlying().empty()) {
    throw InvalidConfig("PostgreSQL Dsn is empty");
  }

  auto settings = settings_.Read();

  if (settings->min_size > settings->max_size) {
    throw InvalidConfig(
        "PostgreSQL pool max size is less than requested initial size");
  }

  LOG_INFO() << (mode == InitMode::kAsync ? "Asynchronously" : "Synchronously")
             << " initializing PostgreSQL connection pool, creating up to "
             << settings->min_size << " connections to "
             << DsnCutPassword(dsn_);

  if (mode == InitMode::kAsync) {
    for (std::size_t i = 0; i < settings->min_size; ++i) {
      connect_task_storage_.Detach(
          Connect(engine::SemaphoreLock{size_semaphore_, std::try_to_lock}));
    }
    LOG_INFO() << "Pool initialization is ongoing";
    StartMaintainTask();
    return;
  }

  std::vector<engine::TaskWithResult<bool>> tasks;
  tasks.reserve(settings->min_size);
  for (std::size_t i = 0; i < settings->min_size; ++i) {
    tasks.push_back(
        Connect(engine::SemaphoreLock{size_semaphore_, std::try_to_lock}));
  }
  for (auto& t : tasks) {
    try {
      const auto success = t.Get();
      if (!success) {
        LOG_ERROR() << "Failed to establish connection to PostgreSQL server";
      }
    } catch (const std::exception& e) {
      LOG_ERROR() << "Failed to establish connection with PostgreSQL server "
                  << DsnCutPassword(dsn_) << ": " << e;
    }
  }

  const auto connections_count = size_semaphore_.UsedApprox();
  if (connections_count < settings->min_size) {
    LOG_WARNING() << "Pool is poorly initialized, "
                  << settings->min_size - connections_count
                  << " connections have not been opened, " << connections_count
                  << " connections are ready to use";
  } else {
    LOG_INFO() << "Pool initialized, " << connections_count
               << " connections are ready to use";
  }

  StartMaintainTask();
}

ConnectionPtr ConnectionPool::Acquire(engine::Deadline deadline) {
  // Obtain smart pointer first to prolong lifetime of this object
  auto shared_this = shared_from_this();

  auto config = GetConfigSource().GetSnapshot();
  CheckDeadlineIsExpired(config);
  ConnectionPtr connection{Pop(deadline), std::move(shared_this)};
  CheckDeadlineIsExpired(config);

  ++stats_.connection.used;
  connection->UpdateDefaultCommandControl();
  return connection;
}

void ConnectionPool::AccountConnectionStats(Connection::Statistics conn_stats) {
  auto now = SteadyClock::now();

  stats_.connection.prepared_statements.GetCurrentCounter().Account(
      conn_stats.prepared_statements_current);

  stats_.transaction.total += conn_stats.trx_total;
  stats_.transaction.commit_total += conn_stats.commit_total;
  stats_.transaction.rollback_total += conn_stats.rollback_total;
  stats_.transaction.out_of_trx_total += conn_stats.out_of_trx;
  stats_.transaction.parse_total += conn_stats.parse_total;
  stats_.transaction.execute_total += conn_stats.execute_total;
  stats_.transaction.reply_total += conn_stats.reply_total;
  stats_.transaction.portal_bind_total += conn_stats.portal_bind_total;
  stats_.transaction.error_execute_total += conn_stats.error_execute_total;
  stats_.transaction.execute_timeout += conn_stats.execute_timeout;
  stats_.transaction.duplicate_prepared_statements +=
      conn_stats.duplicate_prepared_statements;

  stats_.transaction.total_percentile.GetCurrentCounter().Account(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          conn_stats.trx_end_time - conn_stats.trx_start_time)
          .count());
  stats_.transaction.busy_percentile.GetCurrentCounter().Account(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          conn_stats.sum_query_duration)
          .count());
  stats_.transaction.wait_start_percentile.GetCurrentCounter().Account(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          conn_stats.work_start_time - conn_stats.trx_start_time)
          .count());
  stats_.transaction.wait_end_percentile.GetCurrentCounter().Account(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          conn_stats.trx_end_time - conn_stats.last_execute_finish)
          .count());
  stats_.transaction.return_to_pool_percentile.GetCurrentCounter().Account(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now - conn_stats.trx_end_time)
          .count());
}

void ConnectionPool::Release(Connection* connection) {
  UASSERT(connection);
  using DecGuard = USERVER_NAMESPACE::utils::SizeGuard<
      USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint32_t>>;
  DecGuard dg{stats_.connection.used, DecGuard::DontIncrement{}};

  // Grab stats only if connection is not in transaction
  if (!connection->IsInTransaction()) {
    AccountConnectionStats(connection->GetStatsAndReset());
  }

  if (!connection->IsConnected() || connection->IsBroken()) {
    DeleteBrokenConnection(connection);
  } else if (connection->IsIdle()) {
    Push(connection);
  } else {
    // Connection cleanup is done asynchronously while returning control to
    // the user
    close_task_storage_.Detach(engine::CriticalAsyncNoSpan(
        [this, connection, dec_cnt = std::move(dg)] {
          LOG_LIMITED_WARNING()
              << "Released connection in busy state. Trying to clean up...";
          CleanupConnection(connection);
        }));
  }
}

const InstanceStatistics& ConnectionPool::GetStatistics() const {
  auto settings = settings_.Read();
  stats_.connection.active = size_semaphore_.UsedApprox();
  stats_.connection.waiting = wait_count_.load(std::memory_order_relaxed);
  stats_.connection.maximum = settings->max_size;
  stats_.connection.max_queue_size = settings->max_queue_size;
  return stats_;
}

Transaction ConnectionPool::Begin(const TransactionOptions& options,
                                  OptionalCommandControl trx_cmd_ctl) {
  const auto trx_start_time = detail::SteadyClock::now();
  const auto deadline =
      testsuite_pg_ctl_.MakeExecuteDeadline(GetExecuteTimeout(trx_cmd_ctl));
  auto conn = Acquire(deadline);
  UASSERT(conn);
  return Transaction{std::move(conn), options, trx_cmd_ctl, trx_start_time};
}

NonTransaction ConnectionPool::Start(OptionalCommandControl cmd_ctl) {
  const auto start_time = detail::SteadyClock::now();
  const auto deadline =
      testsuite_pg_ctl_.MakeExecuteDeadline(GetExecuteTimeout(cmd_ctl));
  auto conn = Acquire(deadline);
  UASSERT(conn);
  return NonTransaction{std::move(conn), start_time};
}

TimeoutDuration ConnectionPool::GetExecuteTimeout(
    OptionalCommandControl cmd_ctl) const {
  if (cmd_ctl) return cmd_ctl->execute;

  return GetDefaultCommandControl().execute;
}

CommandControl ConnectionPool::GetDefaultCommandControl() const {
  return default_cmd_ctls_.GetDefaultCmdCtl();
}

void ConnectionPool::SetSettings(const PoolSettings& settings) {
  auto max_connections = settings.max_size;
  auto cc_max_connections = cc_max_connections_.load();
  if (cc_max_connections_ > 0 && cc_max_connections < max_connections)
    max_connections = cc_max_connections;

  auto reader = settings_.Read();
  if (*reader == settings) return;
  if (reader->max_size != max_connections) {
    size_semaphore_.SetCapacity(max_connections);
  }
  if (reader->connecting_limit != settings.connecting_limit)
    connecting_semaphore_.SetCapacity(settings.connecting_limit
                                          ? settings.connecting_limit
                                          : kUnlimitedConnecting);

  auto writer = settings_.StartWrite();
  *writer = settings;
  writer->max_size = max_connections;
  writer.Commit();
}

void ConnectionPool::SetConnectionSettings(const ConnectionSettings& settings) {
  auto writer = conn_settings_.StartWrite();
  if (*writer != settings) {
    const auto old_settings = *writer;
    const auto old_version = old_settings.version;
    *writer = settings;
    if (old_settings.RequiresConnectionReset(settings)) {
      writer->version = old_version + 1;
    }
    writer.Commit();
  }
}

void ConnectionPool::SetStatementMetricsSettings(
    const StatementMetricsSettings& settings) {
  sts_.SetSettings(settings);
}

void ConnectionPool::SetMaxConnectionsCc(std::size_t max_connections) {
  cc_max_connections_ = max_connections;
}

dynamic_config::Source ConnectionPool::GetConfigSource() const {
  return config_source_;
}

engine::TaskWithResult<bool> ConnectionPool::Connect(
    engine::SemaphoreLock lock) {
  return engine::AsyncNoSpan([this, size_lock = std::move(lock)]() mutable {
    if (!size_lock) {
      size_lock = engine::SemaphoreLock{size_semaphore_, kConnectingTimeout};
    }
    return DoConnect(std::move(size_lock));
  });
}

bool ConnectionPool::DoConnect(engine::SemaphoreLock size_lock) {
  if (!size_lock) return false;
  LOG_TRACE() << "Creating PostgreSQL connection, current pool size: "
              << size_semaphore_.UsedApprox();
  engine::SemaphoreLock connecting_lock{connecting_semaphore_,
                                        kConnectingTimeout};
  if (!connecting_lock) {
    LOG_LIMITED_WARNING() << "Pool has too many establishing connections";
    return false;
  }
  const uint32_t conn_id = ++stats_.connection.open_total;
  auto conn_settings = conn_settings_.Read();
  std::unique_ptr<Connection> connection;
  Stopwatch st{stats_.connection_percentile};
  try {
    connection = Connection::Connect(
        dsn_, resolver_, bg_task_processor_, close_task_storage_, conn_id,
        *conn_settings, default_cmd_ctls_, testsuite_pg_ctl_, ei_settings_,
        std::move(size_lock));
  } catch (const ConnectionTimeoutError&) {
    // No problem if it's connection error
    ++stats_.connection.error_timeout;
    ++stats_.connection.error_total;
    ++stats_.connection.drop_total;
    ++recent_conn_errors_.GetCurrentCounter();
    return false;
  } catch (const ConnectionError&) {
    // No problem if it's connection error
    ++stats_.connection.error_total;
    ++stats_.connection.drop_total;
    ++recent_conn_errors_.GetCurrentCounter();
    return false;
  } catch (const Error& ex) {
    ++stats_.connection.error_total;
    ++stats_.connection.drop_total;
    LOG_LIMITED_WARNING() << "Connection creation failed with error: " << ex;
    throw;
  }
  LOG_TRACE() << "PostgreSQL connection created";

  // Clean up the statistics and not account it
  [[maybe_unused]] const auto& stats = connection->GetStatsAndReset();

  Push(connection.release());
  return true;
}

void ConnectionPool::TryCreateConnectionAsync() {
  auto conn_settings = conn_settings_.Read();
  // Checking errors is more expensive than incrementing an atomic, so we
  // check it only if we can start a new connection.
  if (recent_conn_errors_.GetStatsForPeriod(kRecentErrorPeriod, true) <
      conn_settings->recent_errors_threshold) {
    engine::SemaphoreLock size_lock{size_semaphore_, std::try_to_lock};
    if (size_lock ||
        connect_task_storage_.ActiveTasksApprox() <= kPendingConnectsMax) {
      connect_task_storage_.Detach(Connect(std::move(size_lock)));
    }
  } else {
    LOG_DEBUG() << "Too many connection errors in recent period";
  }
}

void ConnectionPool::CheckMinPoolSizeUnderflow() {
  auto settings = settings_.Read();
  auto count = size_semaphore_.UsedApprox();
  if (count < settings->min_size) {
    LOG_DEBUG() << "Current pool size is less than min_size (" << count << " < "
                << settings->min_size << "). Create new connection.";
    TryCreateConnectionAsync();
  }
}

void ConnectionPool::Push(Connection* connection) {
  auto conn_settings = conn_settings_.Read();
  if (connection->GetSettings().version < conn_settings->version) {
    DropOutdatedConnection(connection);
  } else if (queue_.push(connection)) {
    conn_available_.NotifyOne();
  } else {
    // TODO Reflect this as a statistics error
    LOG_LIMITED_WARNING()
        << "Couldn't push connection back to the pool. Deleting...";
    DeleteConnection(connection);
  }
}

Connection* ConnectionPool::Pop(engine::Deadline deadline) {
  if (engine::current_task::ShouldCancel()) {
    throw PoolError("Task was cancelled before trying to get a connection");
  }

  if (deadline.IsReached()) {
    ++stats_.connection.error_timeout;
    throw PoolError("Deadline reached before trying to get a connection");
  }
  Stopwatch st{stats_.acquire_percentile};
  Connection* connection = nullptr;
  auto conn_settings = conn_settings_.Read();
  while (queue_.pop(connection)) {
    if (connection->GetSettings().version < conn_settings->version) {
      DropOutdatedConnection(connection);
      continue;
    }
    return connection;
  }

  auto settings = settings_.Read();
  SizeGuard wg(wait_count_);
  if (wg.GetValue() > settings->max_queue_size) {
    ++stats_.queue_size_errors;
    throw PoolError("Wait queue size exceeded");
  }
  // No connections found - create a new one if pool is not exhausted
  LOG_DEBUG() << "No idle connections, waiting for one for "
              << deadline.TimeLeft();

  TryCreateConnectionAsync();

  {
    std::unique_lock<engine::Mutex> lock{wait_mutex_};
    // Wait for a connection
    if (conn_available_.WaitUntil(lock, deadline,
                                  [&] { return queue_.pop(connection); })) {
      return connection;
    }
  }

  if (engine::current_task::ShouldCancel()) {
    throw PoolError("Task was cancelled while waiting for connection");
  }

  ++stats_.pool_exhaust_errors;
  throw PoolError("No available connections found", db_name_);
}

void ConnectionPool::Clear() {
  Connection* connection = nullptr;
  while (queue_.pop(connection)) {
    delete connection;
  }
  close_task_storage_.CancelAndWait();
}

void ConnectionPool::CleanupConnection(Connection* connection) {
  if (cancel_limit_.Obtain()) {
    try {
      connection->CancelAndCleanup(kCleanupTimeout);
      if (connection->IsIdle()) {
        LOG_DEBUG() << "Successfully cleaned up a dirty connection";
        AccountConnectionStats(connection->GetStatsAndReset());
        Push(connection);
        return;
      }
    } catch (const std::exception& e) {
      LOG_WARNING() << "Exception while cleaning up a dirty connection: " << e;
      connection->MarkAsBroken();
    }
  } else {
    // Too many connections are cancelling ATM, we cannot afford running
    // many synchronous calls and/or keep precious connections hanging.
    // Assume a router with sane connection management logic is in place.
    if (connection->Cleanup(kCleanupTimeout)) {
      LOG_DEBUG() << "Successfully finished waiting for a dirty connection "
                     "to clean up itself";
      AccountConnectionStats(connection->GetStatsAndReset());
      Push(connection);
      return;
    }
    if (!connection->IsConnected()) {
      DeleteBrokenConnection(connection);
      return;
    }
  }
  LOG_WARNING() << "Failed to cleanup a dirty connection, deleting...";
  ++stats_.connection.error_total;
  DeleteConnection(connection);
}

void ConnectionPool::DeleteConnection(Connection* connection) {
  delete connection;
  ++stats_.connection.drop_total;
}

void ConnectionPool::DeleteBrokenConnection(Connection* connection) {
  ++stats_.connection.error_total;
  LOG_LIMITED_WARNING() << "Released connection in closed state. Deleting...";
  DeleteConnection(connection);
}

void ConnectionPool::DropOutdatedConnection(Connection* connection) {
  LOG_LIMITED_WARNING() << "Dropping connection with outdated settings";
  DeleteConnection(connection);
}

Connection* ConnectionPool::AcquireImmediate() {
  Connection* conn = nullptr;
  auto conn_settings = conn_settings_.Read();
  while (queue_.pop(conn)) {
    if (conn->GetSettings().version < conn_settings->version) {
      DropOutdatedConnection(conn);
      continue;
    }
    ++stats_.connection.used;
    return conn;
  }
  ++stats_.pool_exhaust_errors;
  return nullptr;
}

void ConnectionPool::MaintainConnections() {
  // No point in doing database roundtrips if there are queries waiting for
  // connections
  if (wait_count_ > 0) {
    LOG_DEBUG() << "No ping required for connection pool "
                << DsnCutPassword(dsn_);
    return;
  }

  LOG_DEBUG() << "Ping connection pool " << DsnCutPassword(dsn_);
  auto stale_connection = true;
  auto count = size_semaphore_.UsedApprox();
  auto drop_left = kIdleDropLimit;
  auto settings = settings_.Read();
  while (count > 0 && stale_connection) {
    try {
      auto deleter = [this](Connection* c) { DeleteConnection(c); };
      std::unique_ptr<Connection, decltype(deleter)> conn(AcquireImmediate(),
                                                          deleter);
      if (!conn) {
        LOG_DEBUG() << "All connections to `" << DsnCutPassword(dsn_)
                    << "` are busy";
        break;
      }
      stale_connection = conn->GetIdleDuration() >= kMaxIdleDuration;
      if (count > settings->min_size && drop_left > 0) {
        --drop_left;
        --stats_.connection.used;
        LOG_DEBUG() << "Drop idle connection to `" << DsnCutPassword(dsn_)
                    << '`';
        // Close synchronously
        conn->Close();
      } else {
        // Cannot use ConnectionPtr here as we must not use
        // shared_from_this() here (this would prolong pool's lifetime and
        // can cause a deadlock on the periodic task). But we can be sure
        // that `this` outlives the task.
        const auto releaser = [this](Connection* c) { Release(c); };
        std::unique_ptr<Connection, decltype(releaser)> capture(conn.release(),
                                                                releaser);
        capture->Ping();
      }
    } catch (const RuntimeError& e) {
      LOG_LIMITED_WARNING() << "Exception while pinging connection to `"
                            << DsnCutPassword(dsn_) << "`: " << e;
    }
    --count;
  }

  // Check and maintain minimum count of connections
  CheckMinPoolSizeUnderflow();
}

void ConnectionPool::StartMaintainTask() {
  using Flags = USERVER_NAMESPACE::utils::PeriodicTask::Flags;

  ping_task_.Start(kMaintainTaskName, {kMaintainInterval, Flags::kStrong},
                   [this] { MaintainConnections(); });
}

void ConnectionPool::StopMaintainTask() { ping_task_.Stop(); }

void ConnectionPool::StopConnectTasks() {
  const auto task_count = connect_task_storage_.ActiveTasksApprox();
  if (task_count) {
    LOG_INFO() << "Waiting for " << task_count
               << " background tasks to complete";
  }
  connect_task_storage_.CancelAndWait();
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
