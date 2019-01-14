#pragma once

#include <cstddef>
#include <memory>

namespace storages {
namespace postgres {

class ConnectionPool;

namespace detail {

class Connection;
class ConnectionPoolImpl;

/// Pointer-like class that controls lifetime of a parent pool by keeping smart
/// pointer to it.
class ConnectionPtr {
 public:
  ConnectionPtr(std::nullptr_t = nullptr);
  ConnectionPtr(std::unique_ptr<Connection> conn);
  ConnectionPtr(Connection* conn, std::shared_ptr<ConnectionPoolImpl> pool);
  ~ConnectionPtr();

  ConnectionPtr(ConnectionPtr&&) noexcept;
  ConnectionPtr& operator=(ConnectionPtr&&) noexcept;

  explicit operator bool() const noexcept;
  Connection* get() const noexcept;

  Connection& operator*() const;
  Connection* operator->() const noexcept;

 private:
  std::unique_ptr<Connection> conn_;
  std::shared_ptr<ConnectionPoolImpl> pool_;
};

}  // namespace detail
}  // namespace postgres
}  // namespace storages
