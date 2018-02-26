#include "pool.hpp"

#include <atomic>

#include <boost/lockfree/queue.hpp>

#include <logging/log.hpp>

#include "wrappers.hpp"

namespace storages {
namespace mongo {

class Pool::Impl {
 public:
  using Connection = ::mongo::DBClientConnection;

  Impl(const std::string& uri, engine::TaskProcessor& task_processor,
       int so_timeout_ms, size_t min_size, size_t max_size);
  ~Impl();

  void Push(Connection* connection);
  Connection* Pop();

  engine::TaskProcessor& GetTaskProcessor();

 private:
  void Clear();
  Connection* Create();

  double so_timeout_sec_;

  ::mongo::ConnectionString cs_;
  engine::TaskProcessor& task_processor_;

  boost::lockfree::queue<Connection*> queue_;
  std::atomic<size_t> size_;
};

Pool::Pool(const std::string& uri, engine::TaskProcessor& task_processor,
           int so_timeout_ms, size_t min_size, size_t max_size)
    : impl_(std::make_unique<Impl>(uri, task_processor, so_timeout_ms, min_size,
                                   max_size)) {}

Pool::~Pool() = default;

std::unique_ptr<Pool::Impl::Connection,
                std::function<void(Pool::Impl::Connection*)>>
Pool::Acquire() {
  return {impl_->Pop(),
          [this](Impl::Connection* connection) { Release(connection); }};
}

void Pool::Release(Impl::Connection* connection) { impl_->Push(connection); }

CollectionWrapper Pool::GetCollection(const std::string& collection_name) {
  return {shared_from_this(), collection_name, impl_->GetTaskProcessor()};
}

Pool::Impl::Impl(const std::string& uri, engine::TaskProcessor& task_processor,
                 int so_timeout_ms, size_t min_size, size_t max_size)
    : task_processor_(task_processor) {
  if (so_timeout_ms <= 0) {
    throw InvalidConfig("invalid so_timeout");
  }
  if (!max_size || min_size > max_size) {
    throw InvalidConfig("invalid pool sizes");
  }

  so_timeout_sec_ = so_timeout_ms / 1000.0;

  std::string parse_error;
  cs_ = ::mongo::ConnectionString::parse(uri, parse_error);
  if (!cs_.isValid()) throw InvalidConfig("bad uri: " + parse_error);
  if (cs_.getServers().size() != 1)
    throw InvalidConfig("bad uri: specify exactly one server");

  try {
    queue_.reserve(max_size);
    LOG_INFO() << "creating" << min_size << " mongo connections";
    for (size_t i = 0; i < min_size; ++i) Push(Create());
  } catch (const std::exception&) {
    Clear();
    throw;
  }
}

Pool::Impl::~Impl() { Clear(); }

void Pool::Impl::Push(Connection* connection) {
  if (!connection) return;
  if (queue_.push(connection)) return;
  delete connection;
  --size_;
}

Pool::Impl::Connection* Pool::Impl::Pop() {
  Connection* connection = nullptr;
  if (queue_.pop(connection)) return connection;
  if (size_ > kCriticalPoolSize)
    throw PoolError("Mongo pool reached critical size: " +
                    std::to_string(size_));

  LOG_INFO() << "Creating extra mongo connection, size: " << size_.load();
  return Create();
}

engine::TaskProcessor& Pool::Impl::GetTaskProcessor() {
  return task_processor_;
}

void Pool::Impl::Clear() {
  Connection* connection = nullptr;
  while (queue_.pop(connection)) delete connection;
}

Pool::Impl::Connection* Pool::Impl::Create() {
  auto connection =
      std::make_unique<Connection>(kAutoReconnect, nullptr, so_timeout_sec_);

  std::string errmsg;
  if (!connection->connect(cs_.getServers().front(), errmsg))
    throw PoolError("Connection to mongo failed: " + errmsg);
  if (!cs_.getUser().empty()) {
    if (!connection->auth(cs_.getDatabase(), cs_.getUser(), cs_.getPassword(),
                          errmsg))
      throw PoolError("Mongo authentication failed: " + errmsg);
  }

  ++size_;
  return connection.release();
}

}  // namespace mongo
}  // namespace storages
