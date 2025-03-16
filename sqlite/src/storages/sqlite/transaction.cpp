#include <memory>
#include <userver/storages/sqlite/transaction.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(infra::ConnectionPtr&& connection,
                         const settings::TransactionOptions& options)
    : connection_{
          std::make_shared<infra::ConnectionPtr>(std::move(connection))} {
  if (connection_ && connection_->IsValid()) {
    (*connection_)->Begin(options);
  }
}

Transaction::Transaction(Transaction&& other) noexcept = default;
Transaction& Transaction::operator=(Transaction&&) noexcept = default;

Transaction::~Transaction() {
  if (connection_ && connection_->IsValid()) {
    try {
      Rollback();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
    }
  }
}

void Transaction::AssertValid() const {
  // TODO: exception or abort?
  UINVARIANT(connection_ && connection_->IsValid(),
             "Transaction accessed after it's been commited");
}

Savepoint Transaction::Save(std::string name) const {
  AssertValid();
  return Savepoint{connection_, std::move(name)};
}

void Transaction::Commit() {
  AssertValid();
  {
    auto connection = std::move(connection_);
    (*connection)->Commit();
  }
}

void Transaction::Rollback() {
  AssertValid();
  {
    auto connection = std::move(connection_);
    (*connection)->Rollback();
  }
}

ResultSet Transaction::DoExecute(settings::OptionalCommandControl optional_cc,
                                 impl::io::ParamsBinderBase& params) const {
  auto prepare_statement = params.GetBindsPtr();
  return (*connection_)->ExecuteCommand(optional_cc, prepare_statement);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
