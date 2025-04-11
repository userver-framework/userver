#include <userver/storages/sqlite/impl/statements_cache.hpp>

#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

StatementsCache::StatementsCache(const NativeHandler& db_handler, std::size_t capacity)
    : db_handler_{db_handler}, cache_(capacity) {
    UASSERT(capacity > 0);
}

StatementsCache::~StatementsCache() = default;

std::shared_ptr<Statement> StatementsCache::PrepareStatement(const std::string& statement) {
    auto* val_ptr = cache_.Get(statement);
    if (val_ptr) {
        return *val_ptr;
    }
    if (cache_.GetSize() == cache_.GetCapacity()) {
        auto statement_to_be_deleted = *cache_.GetLeastUsed();
        UASSERT(statement_to_be_deleted);
        cache_.Erase(statement_to_be_deleted->GetStatementText());
    }
    auto* added_statement = cache_.Emplace(statement, std::make_shared<Statement>(db_handler_, statement));
    UASSERT(added_statement);
    return *added_statement;
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
