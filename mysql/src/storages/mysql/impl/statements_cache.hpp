#pragma once

#include <userver/cache/lru_map.hpp>
#include <userver/utils/str_icase.hpp>

#include <storages/mysql/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class Connection;

class StatementsCache final {
 public:
  StatementsCache(Connection& connection, std::size_t capacity);
  ~StatementsCache();

  Statement& PrepareStatement(const std::string& statement,
                              engine::Deadline deadline);

 private:
  Connection& connection_;

  cache::LruMap<std::string, Statement, utils::StrIcaseHash,
                utils::StrIcaseEqual>
      cache_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
