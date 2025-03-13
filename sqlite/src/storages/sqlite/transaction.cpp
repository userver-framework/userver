#include <userver/storages/sqlite/transaction.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

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

impl::StatementBasePtr Transaction::PrepareStatement(const Query& query) const {
  return (*connection_)->PrepareStatement(query);
}

void Transaction::AssertValid() const {
  // TODO: exception or abort?
  UINVARIANT(connection_->IsValid(),
             "Transaction accessed after it's been commited");
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

ResultSet Transaction::DoExecute(
    settings::OptionalCommandControl optional_cc,
    impl::StatementBasePtr prepare_statement) const {
  return (*connection_)->ExecuteCommand(optional_cc, prepare_statement);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
