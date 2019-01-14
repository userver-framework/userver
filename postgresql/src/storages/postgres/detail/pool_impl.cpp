#include <storages/postgres/detail/pool_impl.hpp>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <storages/postgres/detail/time_types.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {
namespace detail {

namespace {

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

ConnectionPoolImpl::ConnectionPoolImpl(const std::string& dsn,
                                       engine::TaskProcessor& bg_task_processor,
                                       size_t initial_size, size_t max_size)
    : dsn_(dsn),
      bg_task_processor_(bg_task_processor),
      max_size_(max_size),
      queue_(max_size),
      size_(0) {
  if (dsn_.empty()) {
    throw InvalidConfig("PostgreSQL DSN is empty");
  }

  std::vector<engine::TaskWithResult<Connection*>> tasks;
  tasks.reserve(initial_size);
  LOG_INFO() << "Creating " << initial_size << " PostgreSQL connections";
  for (size_t i = 0; i < initial_size; ++i) {
    auto task = engine::Async([this] { return Create(); });
    tasks.push_back(std::move(task));
  }

  for (auto&& task : tasks) {
    try {
      Push(task.Get());
    } catch (const ConnectionError&) {
      // We'll re-create connections later on demand
    } catch (const std::exception& ex) {
      LOG_ERROR() << "PostgreSQL pool pre-population failed: " << ex.what();
      Clear();
      throw;
    }
  }
}

ConnectionPoolImpl::~ConnectionPoolImpl() { Clear(); }

ConnectionPtr ConnectionPoolImpl::Acquire() {
  // Obtain smart pointer first to prolong lifetime of this object
  auto thiz = shared_from_this();
  auto* connection = Pop();
  ++stats_.connection.used;
  return {connection, std::move(thiz)};
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
  LOG_WARNING() << "Released connection in "
                << (connection->IsConnected() ? "busy" : "closed")
                << " state. Deleting...";
  DeleteConnection(connection);
}

const InstanceStatistics& ConnectionPoolImpl::GetStatistics() const {
  stats_.connection.active = size_.load(std::memory_order_relaxed);
  stats_.connection.maximum = max_size_;
  return stats_;
}

Transaction ConnectionPoolImpl::Begin(const TransactionOptions& options) {
  auto trx_start_time = detail::SteadyClock::now();
  auto conn = Acquire();
  assert(conn);
  return Transaction{std::move(conn), options, std::move(trx_start_time)};
}

Connection* ConnectionPoolImpl::Create() {
  SizeGuard sg(size_);

  if (sg.GetSize() > max_size_) {
    ++stats_.pool_error_exhaust_total;
    throw PoolError("PostgreSQL pool reached maximum size: " +
                    std::to_string(max_size_));
  }

  LOG_DEBUG() << "Creating PostgreSQL connection, current pool size: "
              << sg.GetSize();
  const auto conn_id = ++stats_.connection.open_total;
  std::unique_ptr<Connection> connection;
  try {
    connection = Connection::Connect(dsn_, bg_task_processor_, conn_id);
  } catch (const Error&) {
    ++stats_.connection.error_total;
    throw;
  }

  // Clean up the statistics and not account it
  std::ignore = connection->GetStatsAndReset();

  sg.Dismiss();
  return connection.release();
}

void ConnectionPoolImpl::Push(Connection* connection) {
  if (queue_.push(connection)) {
    return;
  }

  LOG_WARNING() << "Couldn't push connection back to the pool. Deleting...";
  DeleteConnection(connection);
}

Connection* ConnectionPoolImpl::Pop() {
  Connection* connection = nullptr;
  if (queue_.pop(connection)) {
    return connection;
  }
  return Create();
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

}  // namespace detail
}  // namespace postgres
}  // namespace storages
