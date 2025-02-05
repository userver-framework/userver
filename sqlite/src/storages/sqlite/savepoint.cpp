#include <userver/storages/sqlite/savepoint.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Savepoint::Savepoint(std::shared_ptr<impl::ConnectionImpl> pimpl,
                     std::string name)
    : pimpl_(std::move(pimpl)), name_(pimpl->PrepareString(std::move(name))) {
  pimpl->Savepoint(name_);
}

Savepoint::Savepoint(Savepoint&& other) noexcept = default;

Savepoint::~Savepoint() {
  try {
    RollbackTo();
    Release();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to auto rollback a savepoint: " << ex.what();
  }
}

void Savepoint::Release() { pimpl_->Release(name_); }

void Savepoint::RollbackTo() { pimpl_->RollbackTo(name_); }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
