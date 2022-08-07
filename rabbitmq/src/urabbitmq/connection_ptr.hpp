#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPool;
class Connection;

class ConnectionPtr {
 public:
  ConnectionPtr(std::shared_ptr<ConnectionPool>&& pool,
                std::unique_ptr<Connection>&& conn);
  ~ConnectionPtr();

  ConnectionPtr(const ConnectionPtr& other) = delete;
  ConnectionPtr(ConnectionPtr&& other) noexcept;

  Connection* operator->();

  void Adopt();

 private:
  void Release();

  std::shared_ptr<ConnectionPool> pool_;
  std::unique_ptr<Connection> conn_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
