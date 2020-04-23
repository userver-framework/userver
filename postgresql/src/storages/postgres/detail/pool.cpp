#include <storages/postgres/detail/pool.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages::postgres::detail {

namespace {

constexpr std::size_t kRecentErrorThreshold = 2;
constexpr std::chrono::seconds kRecentErrorPeriod{15};

// Part of max_pool that can be cancelled at once
constexpr std::size_t kCancelRatio = 2;
constexpr std::chrono::seconds kCancelPeriod{1};

constexpr std::chrono::seconds kCleanupTimeout{1};

constexpr std::chrono::seconds kMaintainInterval{30};
constexpr std::chrono::seconds kMaxIdleDuration{15};
constexpr const char* kMaintainTaskName = "pg_maintain";

// Max idle connections that can be dropped in one run of maintenance task
constexpr auto kIdleDropLimit = 1;

struct Stopwatch {
  using Accumulator = ::utils::statistics::RecentPeriod<Percentile, Percentile,
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
    EmplaceEnabler, Dsn dsn, engine::TaskProcessor& bg_task_processor,
    const PoolSettings& settings, const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings)
    : dsn_{std::move(dsn)},
      settings_{settings},
      conn_settings_{conn_settings},
      bg_task_processor_{bg_task_processor},
      queue_{settings_.max_size},
      size_{std::make_shared<std::atomic<size_t>>(0)},
      wait_count_{0},
      default_cmd_ctl_{default_cmd_ctl},
      has_user_default_cc_{false},
      testsuite_pg_ctl_{testsuite_pg_ctl},
      ei_settings_(std::move(ei_settings)),
      cancel_limit_{std::max(1UL, settings_.max_size / kCancelRatio),
                    kCancelPeriod} {}

ConnectionPool::~ConnectionPool() {
  StopMaintainTask();
  Clear();
}

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    Dsn dsn, engine::TaskProcessor& bg_task_processor,
    const PoolSettings& pool_settings, const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings) {
  auto impl = std::make_shared<ConnectionPool>(
      EmplaceEnabler{}, std::move(dsn), bg_task_processor, pool_settings,
      conn_settings, default_cmd_ctl, testsuite_pg_ctl, std::move(ei_settings));
  // Init() uses shared_from_this for connections and cannot be called from ctor
  impl->Init();
  return impl;
}

void ConnectionPool::Init() {
  if (dsn_.GetUnprotectedRawValue().empty()) {
    throw InvalidConfig("PostgreSQL Dsn is empty");
  }

  if (settings_.min_size > settings_.max_size) {
    throw InvalidConfig(
        "PostgreSQL pool max size is less than requested initial size");
  }

  LOG_INFO() << "Creating " << settings_.min_size
             << " PostgreSQL connections to " << DsnCutPassword(dsn_)
             << (settings_.sync_start ? " sync" : " async");
  if (!settings_.sync_start) {
    for (size_t i = 0; i < settings_.min_size; ++i) {
      Connect(SharedSizeGuard{size_}).Detach();
    }
  } else {
    std::vector<engine::TaskWithResult<bool>> tasks;
    tasks.reserve(settings_.min_size);
    for (size_t i = 0; i < settings_.min_size; ++i) {
      tasks.push_back(Connect(SharedSizeGuard{size_}));
    }
    for (auto& t : tasks) {
      try {
        t.Get();
      } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to establish connection with PostgreSQL server "
                    << DsnCutPassword(dsn_) << ": " << e;
      }
    }
  }
  LOG_INFO() << "Pool initialized";
  StartMaintainTask();
}

ConnectionPtr ConnectionPool::Acquire(engine::Deadline deadline) {
  // Obtain smart pointer first to prolong lifetime of this object
  auto shared_this = shared_from_this();
  ConnectionPtr connection{Pop(deadline), std::move(shared_this)};
  ++stats_.connection.used;
  connection->SetDefaultCommandControl(GetDefaultCommandControl());
  return connection;
}

void ConnectionPool::AccountConnectionStats(Connection::Statistics conn_stats) {
  auto now = SteadyClock::now();

  stats_.transaction.total += conn_stats.trx_total;
  stats_.transaction.commit_total += conn_stats.commit_total;
  stats_.transaction.rollback_total += conn_stats.rollback_total;
  stats_.transaction.out_of_trx_total += conn_stats.out_of_trx;
  stats_.transaction.parse_total += conn_stats.parse_total;
  stats_.transaction.execute_total += conn_stats.execute_total;
  stats_.transaction.reply_total += conn_stats.reply_total;
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
  using DecGuard =
      ::utils::SizeGuard<::utils::statistics::RelaxedCounter<uint32_t>>;
  DecGuard dg{stats_.connection.used, DecGuard::DontIncrement{}};

  // Grab stats only if connection is not in transaction
  if (!connection->IsInTransaction()) {
    AccountConnectionStats(connection->GetStatsAndReset());
  }

  if (connection->IsIdle()) {
    Push(connection);
    return;
  }

  if (!connection->IsConnected()) {
    DeleteBrokenConnection(connection);
  } else {
    // Connection cleanup is done asynchronously while returning control to the
    // user
    engine::impl::CriticalAsync([shared_this = shared_from_this(), connection,
                                 dec_cnt = std::move(dg)] {
      LOG_WARNING()
          << "Released connection in busy state. Trying to clean up...";
      if (shared_this->cancel_limit_.Obtain()) {
        try {
          connection->CancelAndCleanup(kCleanupTimeout);
          if (connection->IsIdle()) {
            LOG_DEBUG() << "Successfully cleaned up a dirty connection";
            shared_this->AccountConnectionStats(connection->GetStatsAndReset());
            shared_this->Push(connection);
            return;
          }
        } catch (const std::exception& e) {
          LOG_WARNING() << "Exception while cleaning up a dirty connection: "
                        << e;
        }
      } else {
        // Too many connections are cancelling ATM, we cannot afford running
        // many synchronous calls and/or keep precious connections hanging.
        // Assume a router with sane connection management logic is in place.
        if (connection->Cleanup(kCleanupTimeout)) {
          LOG_DEBUG() << "Successfully finished waiting for a dirty connection "
                         "to clean up itself";
          shared_this->AccountConnectionStats(connection->GetStatsAndReset());
          shared_this->Push(connection);
          return;
        }
        if (!connection->IsConnected()) {
          shared_this->DeleteBrokenConnection(connection);
          return;
        }
      }
      LOG_WARNING() << "Failed to cleanup a dirty connection, deleting...";
      ++shared_this->stats_.connection.error_total;
      shared_this->DeleteConnection(connection);
    })
        .Detach();
  }
}

const InstanceStatistics& ConnectionPool::GetStatistics() const {
  stats_.connection.active = size_->load(std::memory_order_relaxed);
  stats_.connection.waiting = wait_count_.load(std::memory_order_relaxed);
  stats_.connection.maximum = settings_.max_size;
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
    OptionalCommandControl cmd_ctl) {
  if (cmd_ctl) return cmd_ctl->execute;

  const auto read_ptr = default_cmd_ctl_.Read();
  return read_ptr->execute;
}

void ConnectionPool::SetDefaultCommandControl(
    CommandControl cmd_ctl, DefaultCommandControlSource source) {
  auto writer = default_cmd_ctl_.StartWrite();
  // source must be checked under lock to avoid races
  switch (source) {
    case DefaultCommandControlSource::kGlobalConfig:
      if (has_user_default_cc_) return;
      break;
    case DefaultCommandControlSource::kUser:
      has_user_default_cc_ = true;
      break;
  }
  if (*writer != cmd_ctl) {
    *writer = cmd_ctl;
    writer.Commit();
  }
}

CommandControl ConnectionPool::GetDefaultCommandControl() const {
  return default_cmd_ctl_.ReadCopy();
}

engine::TaskWithResult<bool> ConnectionPool::Connect(
    SharedSizeGuard&& size_guard) {
  return engine::impl::Async([shared_this = shared_from_this(),
                              sg = std::move(size_guard)]() mutable {
    LOG_TRACE() << "Creating PostgreSQL connection, current pool size: "
                << sg.GetValue();
    const uint32_t conn_id = ++shared_this->stats_.connection.open_total;
    std::unique_ptr<Connection> connection;
    Stopwatch st{shared_this->stats_.connection_percentile};
    try {
      auto cmd_ctl = shared_this->default_cmd_ctl_.Read();
      connection = Connection::Connect(
          shared_this->dsn_, shared_this->bg_task_processor_, conn_id,
          shared_this->conn_settings_, *cmd_ctl, shared_this->testsuite_pg_ctl_,
          shared_this->ei_settings_, std::move(sg));
    } catch (const ConnectionTimeoutError&) {
      // No problem if it's connection error
      ++shared_this->stats_.connection.error_timeout;
      ++shared_this->stats_.connection.error_total;
      ++shared_this->stats_.connection.drop_total;
      ++shared_this->recent_conn_errors_.GetCurrentCounter();
      return false;
    } catch (const ConnectionError&) {
      // No problem if it's connection error
      ++shared_this->stats_.connection.error_total;
      ++shared_this->stats_.connection.drop_total;
      ++shared_this->recent_conn_errors_.GetCurrentCounter();
      return false;
    } catch (const Error& ex) {
      ++shared_this->stats_.connection.error_total;
      ++shared_this->stats_.connection.drop_total;
      LOG_WARNING() << "Connection creation failed with error: " << ex;
      throw;
    }
    LOG_TRACE() << "PostgreSQL connection created";

    // Clean up the statistics and not account it
    [[maybe_unused]] const auto& stats = connection->GetStatsAndReset();

    shared_this->Push(connection.release());
    return true;
  });
}

void ConnectionPool::Push(Connection* connection) {
  if (queue_.push(connection)) {
    conn_available_.NotifyOne();
    return;
  }

  // TODO Reflect this as a statistics error
  LOG_WARNING() << "Couldn't push connection back to the pool. Deleting...";
  DeleteConnection(connection);
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
  if (queue_.pop(connection)) {
    return connection;
  }

  SizeGuard wg(wait_count_);
  if (wg.GetValue() > settings_.max_queue_size) {
    ++stats_.queue_size_errors;
    throw PoolError("Wait queue size exceeded");
  }
  // No connections found - create a new one if pool is not exhausted
  auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline.TimeLeft());
  LOG_DEBUG() << "No idle connections, waiting for one for " << timeout.count()
              << "ms";
  {
    SharedSizeGuard sg(size_);
    if (sg.GetValue() <= settings_.max_size) {
      // Checking errors is more expensive than incrementing an atomic, so we
      // check it only if we can start a new connection.
      if (recent_conn_errors_.GetStatsForPeriod(kRecentErrorPeriod, true) <
          kRecentErrorThreshold) {
        // Create a new connection
        Connect(std::move(sg)).Detach();
      } else {
        LOG_DEBUG() << "Too many connection errors in recent period";
      }
    }
  }
  {
    std::unique_lock<engine::Mutex> lock{wait_mutex_};
    // Wait for a connection
    if (conn_available_.WaitUntil(lock, deadline, [&] {
          // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
          return queue_.pop(connection);
        })) {
      return connection;
    }
  }

  if (engine::current_task::ShouldCancel()) {
    throw PoolError("Task was cancelled while waiting for connection");
  }

  ++stats_.pool_exhaust_errors;
  throw PoolError("No available connections found");
}

void ConnectionPool::Clear() {
  Connection* connection = nullptr;
  while (queue_.pop(connection)) {
    delete connection;
  }
}

void ConnectionPool::DeleteConnection(Connection* connection) {
  delete connection;
  ++stats_.connection.drop_total;
}

void ConnectionPool::DeleteBrokenConnection(Connection* connection) {
  ++stats_.connection.error_total;
  LOG_WARNING() << "Released connection in closed state. Deleting...";
  DeleteConnection(connection);
}

Connection* ConnectionPool::AcquireImmediate() {
  Connection* conn = nullptr;
  if (queue_.pop(conn)) {
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
  auto count = size_->load(std::memory_order_relaxed);
  auto drop_left = kIdleDropLimit;
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
      if (count > settings_.min_size && drop_left > 0) {
        --drop_left;
        --stats_.connection.used;
        LOG_DEBUG() << "Drop idle connection to `" << DsnCutPassword(dsn_)
                    << '`';
        // Close synchronously
        conn->Close();
      } else {
        // Recapture the connection pointer to handle errors
        ConnectionPtr capture{conn.release(), shared_from_this()};
        capture->Ping();
      }
    } catch (const RuntimeError& e) {
      LOG_WARNING() << "Exception while pinging connection to `"
                    << DsnCutPassword(dsn_) << "`: " << e;
    }
    --count;
  }

  // TODO Check and maintain minimum count of connections
}

void ConnectionPool::StartMaintainTask() {
  using Flags = ::utils::PeriodicTask::Flags;

  ping_task_.Start(kMaintainTaskName, {kMaintainInterval, Flags::kStrong},
                   [this] { MaintainConnections(); });
}

void ConnectionPool::StopMaintainTask() { ping_task_.Stop(); }

}  // namespace storages::postgres::detail
