#include <userver/storages/sqlite/impl/statements_cache.hpp>

#include <memory>

#include <sqlite3.h>

#include <userver/storages/sqlite/impl/statements.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

StatementsCache::StatementsCache(sqlite3* db_handler, std::size_t capacity)
    : db_handler_{db_handler}, cache_(capacity) {
  UASSERT(capacity > 0);
}

StatementsCache::~StatementsCache() = default;

Statement& StatementsCache::PrepareStatement(const std::string& statement) {
  auto cch = cache_.Lock();
  auto* val_ptr = cch->Get(statement);
  if (val_ptr) {
    return *val_ptr;
  }
  if (cch->GetSize() == cch->GetCapacity()) {
    auto statement_to_be_deleted = cch->GetLeastUsed();
    UASSERT(statement_to_be_deleted);
    cch->Erase(statement_to_be_deleted->GetStatementText());
  }
  auto* added_statement = cch->Emplace(statement, db_handler_, statement);
  UASSERT(added_statement);
  return *added_statement;
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
