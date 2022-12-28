#pragma once

#include <string>

#include <storages/mysql/impl/mysql_result.hpp>
#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class BrokenGuard;
class MySQLConnection;

class MySQLPlainQuery final {
 public:
  MySQLPlainQuery(MySQLConnection& connection, const std::string& query);
  ~MySQLPlainQuery();

  MySQLPlainQuery(const MySQLPlainQuery& other) = delete;
  MySQLPlainQuery(MySQLPlainQuery&& other) noexcept;

  void Execute(BrokenGuard& guard, engine::Deadline deadline);
  MySQLResult FetchResult(BrokenGuard& guard, engine::Deadline deadline);

 private:
  MySQLConnection* connection_;
  const std::string& query_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
