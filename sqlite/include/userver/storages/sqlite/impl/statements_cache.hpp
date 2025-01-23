#pragma once

#include <userver/cache/lru_map.hpp>
#include <userver/utils/str_icase.hpp>

#include <userver/storages/sqlite/impl/statements.hpp>

#include <sqlite3.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Connection;

class StatementsCache final {
 public:
  StatementsCache(sqlite3* db_handler, std::size_t capacity);
  ~StatementsCache();

  Statement& PrepareStatement(const std::string& statement);

 private:
  sqlite3* db_handler_;

  cache::LruMap<std::string, Statement, utils::StrIcaseHash,
                utils::StrIcaseEqual>
      cache_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
