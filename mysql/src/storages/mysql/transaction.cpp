#include <userver/storages/mysql/transaction.hpp>

#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

Transaction::Transaction(infra::ConnectionPtr&& connection,
                         engine::Deadline deadline)
    : connection_{std::move(connection)},
      deadline_{deadline},
      span_{"mysql_transaction"} {
  (*connection_)->ExecutePlain("BEGIN", deadline);
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
  if (connection_->IsValid()) {
    try {
      Rollback();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
    }
  }
}

void Transaction::Commit() {
  AssertValid();
  {
    auto connection = std::move(connection_);
    (*connection)->Commit(deadline_);
  }
}

void Transaction::Rollback() {
  AssertValid();
  {
    auto connection = std::move(connection_);
    (*connection)->Rollback(deadline_);
  }
}

StatementResultSet Transaction::DoExecute(
    OptionalCommandControl, ClusterHostType, const Query& query,
    impl::io::ParamsBinderBase& params,
    std::optional<std::size_t> batch_size) const {
  AssertValid();

  tracing::Span execute_span{"mysql_execute"};

  return StatementResultSet{
      (*connection_)
          ->ExecuteStatement(query.GetStatement(), params,
                             /* TODO : deadline? */ deadline_, batch_size)};
}

void Transaction::AssertValid() const {
  UINVARIANT(connection_->IsValid(),
             "Transaction accessed after it's been committed (or rolled back)");
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
