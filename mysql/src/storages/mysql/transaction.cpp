#include <userver/storages/mysql/transaction.hpp>

#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Transaction::Transaction(infra::ConnectionPtr&& connection)
    : connection_{std::move(connection)} {
  // TODO : deadline?
  (*connection_)->ExecutePlain("BEGIN", {});
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
  if (connection_->IsValid()) {
    try {
      Rollback({} /* TODO : think about deadline here */);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
    }
  }
}

void Transaction::Commit(engine::Deadline deadline) {
  UASSERT(connection_->IsValid());
  {
    auto connection = std::move(connection_);
    (*connection)->Commit(deadline);
  }
}

void Transaction::Rollback(engine::Deadline deadline) {
  UASSERT(connection_->IsValid());
  {
    auto connection = std::move(connection_);
    (*connection)->Rollback(deadline);
  }
}

StatementResultSet Transaction::DoExecute(const std::string& query,
                                          io::ParamsBinderBase& params,
                                          engine::Deadline deadline) {
  AssertValid();
  return StatementResultSet{
      (*connection_)->ExecuteStatement(query, params, deadline, std::nullopt)};
}

void Transaction::DoInsert(const std::string& query,
                           io::ParamsBinderBase& params,
                           engine::Deadline deadline) {
  AssertValid();
  (*connection_)->ExecuteInsert(query, params, deadline);
}

void Transaction::AssertValid() const {
  UINVARIANT(connection_->IsValid(),
             "Transaction accessed after it's been committed (rolled back)");
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
