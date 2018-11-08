#include <storages/postgres/pool.hpp>

#include <atomic>

#include <boost/lockfree/queue.hpp>

#include <logging/log.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {

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

class ConnectionPool::Impl {
 public:
  Impl(const std::string& dsn, engine::TaskProcessor& bg_task_processor,
       size_t initial_size, size_t max_size)
      : dsn_(dsn),
        bg_task_processor_(bg_task_processor),
        max_size_(max_size),
        queue_(max_size),
        size_(0) {
    if (dsn_.empty()) {
      throw InvalidConfig("PostgreSQL DSN is empty");
    }

    try {
      LOG_INFO() << "Creating " << initial_size << " PostgreSQL connections";
      for (size_t i = 0; i < initial_size; ++i) {
        Push(Create());
      }
    } catch (const std::exception& ex) {
      LOG_ERROR() << "PostgreSQL pool pre-population failed: " << ex.what();
      Clear();
      throw;
    }
  }

  ~Impl() { Clear(); }

  detail::ConnectionPtr Acquire() {
    return {Pop(),
            [this](detail::Connection* connection) { Release(connection); }};
  }

  void Release(detail::Connection* connection) {
    assert(connection);
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

  Transaction Begin(const TransactionOptions& options) {
    auto conn = Acquire();
    return Transaction{std::move(conn), options};
  }

 private:
  detail::Connection* Create() {
    SizeGuard sg(size_);

    if (sg.GetSize() > max_size_) {
      throw PoolError("PostgreSQL pool reached maximum size: " +
                      std::to_string(max_size_));
    }

    LOG_DEBUG() << "Creating PostgreSQL connection, current pool size: "
                << sg.GetSize();
    std::unique_ptr<detail::Connection> connection =
        detail::Connection::Connect(dsn_, bg_task_processor_);

    sg.Dismiss();
    return connection.release();
  }

  void Push(detail::Connection* connection) {
    if (queue_.push(connection)) {
      return;
    }

    LOG_WARNING() << "Couldn't push connection back to the pool. Deleting...";
    DeleteConnection(connection);
  }

  detail::Connection* Pop() {
    detail::Connection* connection = nullptr;
    if (queue_.pop(connection)) {
      return connection;
    }
    return Create();
  }

  void Clear() {
    detail::Connection* connection = nullptr;
    while (queue_.pop(connection)) {
      delete connection;
    }
  }

  void DeleteConnection(detail::Connection* connection) {
    delete connection;
    --size_;
  }

 private:
  std::string dsn_;
  engine::TaskProcessor& bg_task_processor_;
  size_t max_size_;
  boost::lockfree::queue<detail::Connection*> queue_;
  std::atomic<size_t> size_;
};

ConnectionPool::ConnectionPool(const std::string& dsn,
                               engine::TaskProcessor& bg_task_processor,
                               size_t min_size, size_t max_size) {
  pimpl_ = std::make_unique<Impl>(dsn, bg_task_processor, min_size, max_size);
}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionPool(ConnectionPool&&) noexcept = default;

ConnectionPool& ConnectionPool::operator=(ConnectionPool&&) = default;

detail::ConnectionPtr ConnectionPool::GetConnection() {
  return pimpl_->Acquire();
}

Transaction ConnectionPool::Begin(const TransactionOptions& options) {
  return pimpl_->Begin(options);
}

}  // namespace postgres
}  // namespace storages
