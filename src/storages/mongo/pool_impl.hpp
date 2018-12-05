#pragma once

#include <atomic>
#include <memory>

#include <boost/lockfree/queue.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <engine/task/task_processor.hpp>
#include <storages/mongo/pool_config.hpp>

namespace storages {
namespace mongo {
namespace impl {

class PoolImpl {
 public:
  using Connection = mongocxx::client;
  using ConnectionPtr =
      std::unique_ptr<Connection, std::function<void(Connection*)>>;

  PoolImpl(engine::TaskProcessor& task_processor, const std::string& uri,
           const PoolConfig& config);
  ~PoolImpl();

  ConnectionPtr Acquire();

  const std::string& GetDefaultDatabaseName() const;
  engine::TaskProcessor& GetTaskProcessor();

 private:
  void Push(Connection* connection);
  Connection* Pop();

  Connection* Create();
  void Clear();

  engine::TaskProcessor& task_processor_;
  mongocxx::uri uri_;
  std::string default_database_name_;

  boost::lockfree::queue<Connection*> queue_;
  std::atomic<size_t> size_;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
