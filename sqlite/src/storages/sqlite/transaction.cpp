#include <userver/storages/sqlite/transaction.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(infra::ConnectionPtr&& connection,
                         const settings::TransactionOptions& options)
    : connection_{std::move(connection)} {
  if (connection_->IsValid()) {
    (*connection_)->Begin(options);
  }
}

Transaction::Transaction(Transaction&& other) noexcept = default;
Transaction& Transaction::operator=(Transaction&&) noexcept = default;

Transaction::~Transaction() {
  if (connection_->IsValid()) {
    try {
      Rollback();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
    }
  }
}

void Transaction::AssertValid() const {
  UINVARIANT(connection_->IsValid(),
             "Transaction accessed after it's been released");
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

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
