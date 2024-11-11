#include <userver/storages/sqlite/transaction.hpp>

#include "userver/logging/log.hpp"

#include "userver/storages/sqlite/query.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction() = default;

Transaction::Transaction(const Transaction& other) = default;

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
  try {
    Rollback();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
  }
}

void Transaction::Commit() {}

void Transaction::Rollback() {}

ResultSet Transaction::DoExecute(const Query& query [[maybe_unused]]) const {
  return ResultSet{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
