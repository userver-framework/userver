#pragma once

#include <cstddef>
#include <memory>

namespace storages::postgres::detail {

class Connection;
class ConnectionPool;

/// Pointer-like class that controls lifetime of a parent pool by keeping smart
/// pointer to it.
class ConnectionPtr {
 public:
  explicit ConnectionPtr(std::unique_ptr<Connection>&& conn);
  ConnectionPtr(Connection* conn, std::shared_ptr<ConnectionPool>&& pool);
  ~ConnectionPtr();

  ConnectionPtr(ConnectionPtr&&) noexcept;
  ConnectionPtr& operator=(ConnectionPtr&&) noexcept;

  explicit operator bool() const noexcept;
  Connection* get() const noexcept;

  Connection& operator*() const;
  Connection* operator->() const noexcept;

 private:
  void Reset(std::unique_ptr<Connection> conn,
             std::shared_ptr<ConnectionPool> pool);
  void Release();

  std::shared_ptr<ConnectionPool> pool_;
  std::unique_ptr<Connection> conn_;
};

}  // namespace storages::postgres::detail
