#include <storages/mysql/impl/statements_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

constexpr std::chrono::milliseconds kDefaultStatementDeleteTimeout{200};

}

StatementsCache::StatementsCache(Connection& connection, std::size_t capacity)
    : connection_{connection}, cache_{capacity} {
  UASSERT(capacity > 0);
}

StatementsCache::~StatementsCache() {
  while (auto* least_used = cache_.GetLeastUsed()) {
    least_used->SetDestructionDeadline(
        engine::Deadline::FromDuration(kDefaultStatementDeleteTimeout));
    cache_.Erase(least_used->GetStatementText());
  }

  UASSERT(cache_.GetSize() == 0);
}

Statement& StatementsCache::PrepareStatement(const std::string& statement,
                                             engine::Deadline deadline) {
  auto* statement_ptr = cache_.Get(statement);
  if (statement_ptr) {
    return *statement_ptr;
  }

  // key is not in cache, check if insertion will overflow and set destruction
  // deadline if it's the case
  if (cache_.GetSize() == cache_.GetCapacity()) {
    auto* statement_to_be_deleted = cache_.GetLeastUsed();
    UASSERT(statement_to_be_deleted);

    statement_to_be_deleted->SetDestructionDeadline(deadline);
  }

  auto* added_statement =
      cache_.Emplace(statement, connection_, statement, deadline);
  UASSERT(added_statement);
  return *added_statement;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
