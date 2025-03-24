#include <userver/storages/sqlite/savepoint.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Savepoint::Savepoint(infra::ConnectionPtr&& connection, std::string name)
    : connection_{
          std::make_shared<infra::ConnectionPtr>(std::move(connection))} {
  if (connection_ && connection_->IsValid()) {
    name_ = (*connection_)->PrepareString(name);
    (*connection_)->Savepoint(name_);
  }
}

Savepoint::Savepoint(std::shared_ptr<infra::ConnectionPtr> shared_connection,
                     std::string name)
    : connection_(std::move(shared_connection)) {
  if (connection_ && connection_->IsValid()) {
    name_ = (*connection_)->PrepareString(name);
    (*connection_)->Savepoint(name_);
  }
}

Savepoint::Savepoint(Savepoint&& other) noexcept = default;

Savepoint& Savepoint::operator=(Savepoint&&) noexcept = default;

Savepoint::~Savepoint() {
  if (connection_ && connection_->IsValid()) {
    try {
      RollbackTo();
      Release();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to auto rollback a savepoint: " << ex.what();
    }
  }
}

void Savepoint::AssertValid() const {
  // TODO: exception or abort?
  UINVARIANT(connection_ && connection_->IsValid(),
             "Savepoint accessed after it's been released");
}

void Savepoint::Release() {
  AssertValid();
  {
    auto connection = std::move(connection_);
    (*connection)->Release(name_);
  }
}

void Savepoint::RollbackTo() {
  AssertValid();
  (*connection_)->RollbackTo(name_);
}

Savepoint Savepoint::Save(std::string name) const {
  AssertValid();
  return Savepoint{connection_, std::move(name)};
}

ResultSet Savepoint::DoExecute(impl::io::ParamsBinderBase& params) const {
  auto prepare_statement = params.GetBindsPtr();
  return (*connection_)->ExecuteCommand(prepare_statement);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
