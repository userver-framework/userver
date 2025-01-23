#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include "userver/logging/log.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

StatementsCache::StatementsCache(sqlite3* db_handler, std::size_t capacity)
    : db_handler_{db_handler}, cache_{capacity} {
  UASSERT(capacity > 0);
}

StatementsCache::~StatementsCache() {
  while (auto* least_used = cache_.GetLeastUsed()) {
    cache_.Erase(least_used->GetStatementText());
  }

  UASSERT(cache_.GetSize() == 0);
}

Statement& StatementsCache::PrepareStatement(const std::string& statement) {
  auto* statement_ptr = cache_.Get(statement);
  if (statement_ptr) {
    return *statement_ptr;
  }

  if (cache_.GetSize() == cache_.GetCapacity()) {
    auto* statement_to_be_deleted = cache_.GetLeastUsed();
    UASSERT(statement_to_be_deleted);
    cache_.Erase(statement_to_be_deleted->GetStatementText());
  }

  auto* added_statement = cache_.Emplace(statement, db_handler_, statement);
  return *added_statement;
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
