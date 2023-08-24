#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

class Connection;
class PoolImpl;

class ConnectionPtr final {
 public:
  ConnectionPtr(std::shared_ptr<PoolImpl>&&, Connection*);
  ~ConnectionPtr() noexcept;

  ConnectionPtr(ConnectionPtr&&) noexcept;

  Connection& operator*() const;
  Connection* operator->() const noexcept;

 private:
  void Release() noexcept;

  std::shared_ptr<PoolImpl> pool_;
  std::unique_ptr<Connection> conn_;
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
