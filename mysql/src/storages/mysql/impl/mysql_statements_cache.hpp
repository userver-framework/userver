#pragma once

#include <list>
#include <memory>
#include <unordered_map>

#include <userver/utils/str_icase.hpp>

#include <storages/mysql/impl/mysql_statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLConnection;

class MySQLStatementsCache final {
 public:
  MySQLStatementsCache(MySQLConnection& connection, std::size_t capacity);
  ~MySQLStatementsCache();

  MySQLStatement& PrepareStatement(const std::string& statement,
                                   engine::Deadline deadline);

 private:
  MySQLConnection& connection_;

  using List = std::list<std::pair<std::string, MySQLStatement>>;

  std::size_t capacity_;
  List queue_;
  std::unordered_map<std::string, List::iterator, utils::StrIcaseHash,
                     utils::StrIcaseEqual>
      lookup_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
