#include <storages/postgres/detail/pool_impl.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

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

ConnectionPoolImpl::ConnectionPoolImpl(const std::string& dsn,
                                       engine::TaskProcessor& bg_task_processor,
                                       size_t max_size,
                                       CommandControl default_cmd_ctl)
    : dsn_(dsn),
      bg_task_processor_(bg_task_processor),
      max_size_(max_size),
      queue_(max_size),
      size_(0),
      wait_count_{0},
      default_cmd_ctl_{
          std::make_shared<const CommandControl>(std::move(default_cmd_ctl))} {}

ConnectionPoolImpl::~ConnectionPoolImpl() { Clear(); }

std::shared_ptr<ConnectionPoolImpl> ConnectionPoolImpl::Create(
    const std::string& dsn, engine::TaskProcessor& bg_task_processor,
    size_t initial_size, size_t max_size, CommandControl default_cmd_ctl) {
  // structure to call constructor of ConnectionPoolImpl that shouldn't be
  // accessible in public interface
  struct ImplForConstruction : ConnectionPoolImpl {
    ImplForConstruction(const std::string& dsn,
                        engine::TaskProcessor& bg_task_processor,
                        size_t max_size, CommandControl default_cmd_ctl)
        : ConnectionPoolImpl(dsn, bg_task_processor, max_size,
                             std::move(default_cmd_ctl)) {}
  };

  auto impl = std::make_shared<ImplForConstruction>(dsn, bg_task_processor,
                                                    max_size, default_cmd_ctl);
  impl->Init(initial_size);
  return impl;
}

void ConnectionPoolImpl::Init(size_t initial_size) {
  if (dsn_.empty()) {
    throw InvalidConfig("PostgreSQL DSN is empty");
  }

  if (initial_size > max_size_) {
    throw InvalidConfig(
        "PostgreSQL pool max size is less than requested initial size");
  }

  LOG_INFO() << "Creating " << initial_size << " PostgreSQL connections";
  for (size_t i = 0; i < initial_size; ++i) {
    Connect(SizeGuard{size_}).Detach();
  }
  LOG_INFO() << "Pool initialized";
}

ConnectionPtr ConnectionPoolImpl::Acquire(engine::Deadline deadline) {
  // Obtain smart pointer first to prolong lifetime of this object
  auto shared_this = shared_from_this();
  auto* connection = Pop(deadline);
  ++stats_.connection.used;
  connection->SetDefaultCommandControl(*default_cmd_ctl_.Get());
  return {connection, std::move(shared_this)};
}

void ConnectionPoolImpl::AccountConnectionStats(
    Connection::Statistics conn_stats) {
  auto now = SteadyClock::now();

  stats_.transaction.total += conn_stats.trx_total;
  stats_.transaction.commit_total += conn_stats.commit_total;
  stats_.transaction.rollback_total += conn_stats.rollback_total;
  stats_.transaction.out_of_trx_total += conn_stats.out_of_trx;
  stats_.transaction.parse_total += conn_stats.parse_total;
  stats_.transaction.execute_total += conn_stats.execute_total;
  stats_.transaction.reply_total += conn_stats.reply_total;
  stats_.transaction.bin_reply_total += conn_stats.bin_reply_total;
  stats_.transaction.error_execute_total += conn_stats.error_execute_total;
  stats_.transaction.execute_timeout += conn_stats.execute_timeout;

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

void ConnectionPoolImpl::Release(Connection* connection) {
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

  // TODO: determine connection states that are allowed here
  if (!connection->IsConnected()) {
    ++stats_.connection.error_total;
    LOG_WARNING() << "Released connection in closed state. Deleting...";
    DeleteConnection(connection);
  } else {
    // Connection cleanup is done asynchronously while returning control to the
    // user
    engine::impl::CriticalAsync([shared_this = shared_from_this(), connection,
                                 dec_cnt = std::move(dg)] {
      LOG_WARNING()
          << "Released connection in busy state. Trying to clean up...";
      auto cmd_ctl = shared_this->default_cmd_ctl_.Get();
      try {
        connection->Cleanup(cmd_ctl->network * 10);
        if (connection->IsIdle()) {
          LOG_DEBUG() << "Succesfully cleaned up dirty connection";
          shared_this->AccountConnectionStats(connection->GetStatsAndReset());
          shared_this->Push(connection);
          return;
        }
      } catch (const std::exception& e) {
        LOG_WARNING() << "Exception while cleaning up a dirty connection: "
                      << e;
      }
      LOG_WARNING() << "Failed to cleanup a dirty connection, deleting...";
      ++shared_this->stats_.connection.error_total;
      shared_this->DeleteConnection(connection);
    })
        .Detach();
  }
}

const InstanceStatistics& ConnectionPoolImpl::GetStatistics() const {
  stats_.connection.active = size_.load(std::memory_order_relaxed);
  stats_.connection.waiting = wait_count_.load(std::memory_order_relaxed);
  stats_.connection.maximum = max_size_;
  return stats_;
}

Transaction ConnectionPoolImpl::Begin(const TransactionOptions& options,
                                      engine::Deadline deadline,
                                      OptionalCommandControl trx_cmd_ctl) {
  auto trx_start_time = detail::SteadyClock::now();
  auto conn = Acquire(deadline);
  UASSERT(conn);
  return Transaction{std::move(conn), options, trx_cmd_ctl,
                     std::move(trx_start_time)};
}

NonTransaction ConnectionPoolImpl::Start(engine::Deadline deadline) {
  auto start_time = detail::SteadyClock::now();
  auto conn = Acquire(deadline);
  UASSERT(conn);
  return NonTransaction{std::move(conn), std::move(deadline),
                        std::move(start_time)};
}

engine::TaskWithResult<bool> ConnectionPoolImpl::Connect(
    SizeGuard&& size_guard) {
  return engine::impl::Async(
      [shared_this = shared_from_this(), sg = std::move(size_guard)]() mutable {
        LOG_TRACE() << "Creating PostgreSQL connection, current pool size: "
                    << sg.GetSize();
        const uint32_t conn_id = ++shared_this->stats_.connection.open_total;
        std::unique_ptr<Connection> connection;
        Stopwatch st{shared_this->stats_.connection_percentile};
        try {
          auto cmd_ctl = shared_this->default_cmd_ctl_.Get();
          connection = Connection::Connect(shared_this->dsn_,
                                           shared_this->bg_task_processor_,
                                           conn_id, *cmd_ctl);
        } catch (const ConnectionTimeoutError&) {
          // No problem if it's connection error
          ++shared_this->stats_.connection.error_timeout;
          ++shared_this->stats_.connection.error_total;
          ++shared_this->stats_.connection.drop_total;
          return false;
        } catch (const ConnectionError&) {
          // No problem if it's connection error
          ++shared_this->stats_.connection.error_total;
          ++shared_this->stats_.connection.drop_total;
          return false;
        } catch (const Error& ex) {
          ++shared_this->stats_.connection.error_total;
          ++shared_this->stats_.connection.drop_total;
          LOG_ERROR() << "Connection creation failed with error: " << ex;
          throw;
        }
        LOG_TRACE() << "PostgreSQL connection created";

        // Clean up the statistics and not account it
        [[maybe_unused]] const auto& stats = connection->GetStatsAndReset();

        sg.Dismiss();
        shared_this->Push(connection.release());
        return true;
      });
}

void ConnectionPoolImpl::Push(Connection* connection) {
  if (queue_.push(connection)) {
    conn_available_.NotifyOne();
    return;
  }

  // TODO Reflect this as a statistics error
  LOG_WARNING() << "Couldn't push connection back to the pool. Deleting...";
  DeleteConnection(connection);
}

Connection* ConnectionPoolImpl::Pop(engine::Deadline deadline) {
  if (deadline.IsReached()) {
    ++stats_.connection.error_timeout;
    throw PoolError("Deadline reached before trying to get a connection");
  }
  Stopwatch st{stats_.acquire_percentile};
  auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline.TimeLeft());
  Connection* connection = nullptr;
  if (queue_.pop(connection)) {
    return connection;
  }

  // No connections found - create a new one if pool is not exhausted
  LOG_DEBUG() << "No idle connections, try to get one in " << timeout.count()
              << "ms";
  SizeGuard wg(wait_count_);
  {
    SizeGuard sg(size_);
    if (sg.GetSize() <= max_size_) {
      // Create a new connection
      Connect(std::move(sg)).Detach();
    }
  }
  {
    std::unique_lock<engine::Mutex> lock{wait_mutex_};
    // Wait for a connection
    if (conn_available_.WaitUntil(lock, deadline,
                                  [&] { return queue_.pop(connection); })) {
      return connection;
    }
  }

  ++stats_.pool_error_exhaust_total;
  throw PoolError("No available connections found");
}

void ConnectionPoolImpl::Clear() {
  Connection* connection = nullptr;
  while (queue_.pop(connection)) {
    delete connection;
  }
}

void ConnectionPoolImpl::DeleteConnection(Connection* connection) {
  delete connection;
  --size_;
  ++stats_.connection.drop_total;
}

void ConnectionPoolImpl::SetDefaultCommandControl(CommandControl cmd_ctl) {
  default_cmd_ctl_.Set(std::make_shared<const CommandControl>(cmd_ctl));
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
