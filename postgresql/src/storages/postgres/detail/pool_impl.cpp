#include <storages/postgres/detail/pool_impl.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <logging/log.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

// TODO Move constants to config
// Time to wait for connection to become available
const std::chrono::seconds kConnWaitTimeout(2);
// Minimal duration of connection availability check
const std::chrono::milliseconds kConnCheckDuration(20);

struct SizeGuard {
  SizeGuard(std::atomic<size_t>& size)
      : size_(size), cur_size_(++size_), dismissed_(false) {}

  ~SizeGuard() {
    if (!dismissed_) {
      --size_;
    }
  }

  void Dismiss() { dismissed_ = true; }
  size_t GetSize() const { return cur_size_; }

 private:
  std::atomic<size_t>& size_;
  size_t cur_size_;
  bool dismissed_;
};

}  // namespace

template <typename T>
struct TaskDetachGuard {
  TaskDetachGuard(engine::TaskWithResult<T> task) : task_(std::move(task)) {}

  ~TaskDetachGuard() {
    if (task_.IsValid()) {
      std::move(task_).Detach();
    }
  }

  engine::TaskWithResult<T>& Get() { return task_; }

 private:
  engine::TaskWithResult<T> task_;
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
    Create().Detach();
  }
}

ConnectionPtr ConnectionPoolImpl::Acquire() {
  // Obtain smart pointer first to prolong lifetime of this object
  auto shared_this = shared_from_this();
  auto* connection = Pop();
  ++stats_.connection.used;
  connection->SetDefaultCommandControl(*default_cmd_ctl_.Get());
  return {connection, std::move(shared_this)};
}

void ConnectionPoolImpl::Release(Connection* connection) {
  assert(connection);
  --stats_.connection.used;

  // Grab stats only if connection is not in transaction
  if (!connection->IsInTransaction()) {
    const auto conn_stats = connection->GetStatsAndReset();

    stats_.transaction.total += conn_stats.trx_total;
    stats_.transaction.commit_total += conn_stats.commit_total;
    stats_.transaction.rollback_total += conn_stats.rollback_total;
    stats_.transaction.out_of_trx_total += conn_stats.out_of_trx;
    stats_.transaction.parse_total += conn_stats.parse_total;
    stats_.transaction.execute_total += conn_stats.execute_total;
    stats_.transaction.reply_total += conn_stats.reply_total;
    stats_.transaction.bin_reply_total += conn_stats.bin_reply_total;
    stats_.transaction.error_execute_total += conn_stats.error_execute_total;

    stats_.transaction.total_percentile.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            conn_stats.trx_end_time - conn_stats.trx_start_time)
            .count());
    stats_.transaction.busy_percentile.GetCurrentCounter().Account(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            conn_stats.sum_query_duration)
            .count());
  }

  if (connection->IsIdle()) {
    Push(connection);
    return;
  }

  // TODO: determine connection states that are allowed here
  ++stats_.connection.error_total;
  if (!connection->IsConnected()) {
    LOG_WARNING() << "Released connection in closed state. Deleting...";
    DeleteConnection(connection);
  } else {
    // Connection cleanup is done asynchronously while returning control to the
    // user
    engine::impl::CriticalAsync(
        [ shared_this = shared_from_this(), connection ] {
          LOG_WARNING()
              << "Released connection in busy state. Trying to clean up...";
          auto cmd_ctl = shared_this->default_cmd_ctl_.Get();
          try {
            connection->Cleanup(cmd_ctl->network * 10);
            if (connection->IsIdle()) {
              LOG_DEBUG() << "Succesfully cleaned up dirty connection";
              shared_this->Push(connection);
              return;
            }
          } catch (const std::exception& e) {
            LOG_WARNING() << "Exception while cleaning up a dirty connection: "
                          << e.what();
          }
          LOG_WARNING() << "Failed to cleanup a dirty connection, deleting...";
          shared_this->DeleteConnection(connection);
        })
        .Detach();
  }
}

const InstanceStatistics& ConnectionPoolImpl::GetStatistics() const {
  stats_.connection.active = size_.load(std::memory_order_relaxed);
  stats_.connection.maximum = max_size_;
  return stats_;
}

Transaction ConnectionPoolImpl::Begin(const TransactionOptions& options,
                                      OptionalCommandControl trx_cmd_ctl) {
  auto trx_start_time = detail::SteadyClock::now();
  auto conn = Acquire();
  assert(conn);
  return Transaction{std::move(conn), options, trx_cmd_ctl,
                     std::move(trx_start_time)};
}

NonTransaction ConnectionPoolImpl::Start() {
  auto start_time = detail::SteadyClock::now();
  auto conn = Acquire();
  assert(conn);
  return NonTransaction{std::move(conn), std::move(start_time)};
}

engine::TaskWithResult<bool> ConnectionPoolImpl::Create() {
  return engine::impl::Async([shared_this = shared_from_this()] {
    SizeGuard sg(shared_this->size_);

    if (sg.GetSize() > shared_this->max_size_) {
      LOG_WARNING() << "PostgreSQL pool reached maximum size: "
                    << shared_this->max_size_;
      return false;
    }

    LOG_TRACE() << "Creating PostgreSQL connection, current pool size: "
                << sg.GetSize();
    const decltype(
        shared_this->stats_.connection.open_total)::ValueType conn_id =
        ++shared_this->stats_.connection.open_total;
    std::unique_ptr<Connection> connection;
    try {
      auto cmd_ctl = shared_this->default_cmd_ctl_.Get();
      connection = Connection::Connect(shared_this->dsn_,
                                       shared_this->bg_task_processor_, conn_id,
                                       *cmd_ctl);
    } catch (const ConnectionError&) {
      // No problem if it's connection error
      ++shared_this->stats_.connection.error_total;
      return false;
    } catch (const Error& ex) {
      ++shared_this->stats_.connection.error_total;
      LOG_ERROR() << "Connection creation failed with error: " << ex.what();
      throw;
    }
    LOG_TRACE() << "PostgreSQL connection created";

    // Clean up the statistics and not account it
    std::ignore = connection->GetStatsAndReset();

    sg.Dismiss();
    shared_this->Push(connection.release());
    return true;
  });
}

void ConnectionPoolImpl::Push(Connection* connection) {
  if (queue_.push(connection)) {
    return;
  }

  // TODO Reflect this as a statistics error
  LOG_WARNING() << "Couldn't push connection back to the pool. Deleting...";
  DeleteConnection(connection);
}

Connection* ConnectionPoolImpl::Pop() {
  Connection* connection = nullptr;
  if (queue_.pop(connection)) {
    return connection;
  }

  // No connections found - need to create new one and wait for connection to
  // become available in the queue
  const auto conn_wait_end_point =
      detail::SteadyClock::now() + kConnWaitTimeout;
  TaskDetachGuard task_guard(Create());
  auto& create_task = task_guard.Get();

  do {
    const auto next_point =
        std::min(std::chrono::steady_clock::now() + kConnCheckDuration,
                 conn_wait_end_point);
    // TODO Need MPMC queue with wait on empty capability
    engine::SleepUntil(next_point);

    bool create_again = false;
    if (create_task.IsValid() && create_task.IsFinished()) {
      // If connection has been created but we failed to get it from the queue
      // - we'll need to create connection again
      create_again = create_task.Get();
      create_task = {};
    }

    if (queue_.pop(connection)) {
      return connection;
    }

    if (create_again) {
      LOG_DEBUG() << "Connection has been created, but stolen. "
                     "Re-creating again...";
      create_task = Create();
    }
  } while (detail::SteadyClock::now() < conn_wait_end_point);

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
