#pragma once

#include <memory>

#include <sqlite3.h>

#include <userver/cache/lru_map.hpp>
#include <userver/storages/sqlite/impl/statements.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Connection;

class StatementsCache final {
 public:
  StatementsCache(sqlite3* db_handler, std::size_t capacity);
  ~StatementsCache();

  StatementsCache(StatementsCache&&) noexcept = default;
  StatementsCache(const StatementsCache&) = delete;

  StatementsCache& operator=(StatementsCache&&) noexcept = default;
  StatementsCache& operator=(const StatementsCache&) = delete;

  // TODO: Why we can't use Statement& here?
  // CRITICAL <userver> ERROR at
  // userver/universal/include/userver/cache/impl/lru.hpp:344:InsertNode.
  // Assertion 'ok' failed
  std::shared_ptr<Statement> PrepareStatement(const std::string& statement);

 private:
  sqlite3* db_handler_;

  cache::LruMap<std::string, std::shared_ptr<Statement>, utils::StrIcaseHash,
                utils::StrIcaseEqual>
      cache_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
