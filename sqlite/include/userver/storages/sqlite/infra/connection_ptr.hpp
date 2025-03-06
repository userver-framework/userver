#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace impl {
class ConnectionImpl;
}

namespace infra {

class Pool;

/// Pointer-like class that controls lifetime of a parent pool by keeping smart
/// pointer to it.
class ConnectionPtr {
 public:
  ConnectionPtr(std::shared_ptr<Pool>&& pool,
                std::unique_ptr<impl::ConnectionImpl>&& connection);
  ~ConnectionPtr();

  ConnectionPtr(ConnectionPtr&&) noexcept;
  ConnectionPtr& operator=(ConnectionPtr&&) noexcept;

  bool IsValid() const noexcept;
  impl::ConnectionImpl* get() const noexcept;

  impl::ConnectionImpl& operator*() const;
  impl::ConnectionImpl* operator->() const noexcept;

 private:
  void Reset(std::unique_ptr<impl::ConnectionImpl> conn,
             std::shared_ptr<Pool> pool);
  void Release();

  std::shared_ptr<Pool> pool_;
  std::unique_ptr<impl::ConnectionImpl> conn_;
};

}  // namespace infra

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
