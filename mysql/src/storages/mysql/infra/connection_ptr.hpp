#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class MySQLConnection;
}

namespace infra {

class Pool;

class ConnectionPtr final {
 public:
  ConnectionPtr(std::shared_ptr<Pool>&& pool,
                std::unique_ptr<impl::MySQLConnection>&& connection);
  ~ConnectionPtr();

  ConnectionPtr(ConnectionPtr&& other) noexcept;

  impl::MySQLConnection& operator*() const;
  impl::MySQLConnection* operator->() const noexcept;

  bool IsValid() const;

 private:
  void Release() noexcept;

  std::shared_ptr<Pool> pool_;
  std::unique_ptr<impl::MySQLConnection> connection_;
};

}  // namespace infra

}  // namespace storages::mysql

USERVER_NAMESPACE_END
