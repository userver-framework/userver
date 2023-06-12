#pragma once

#include <string>

#include <storages/mysql/impl/query_result.hpp>
#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class BrokenGuard;
class Connection;

class PlainQuery final {
 public:
  PlainQuery(Connection& connection, const std::string& query);
  ~PlainQuery();

  PlainQuery(const PlainQuery& other) = delete;
  PlainQuery(PlainQuery&& other) noexcept;

  void Execute(engine::Deadline deadline);
  QueryResult FetchResult(engine::Deadline deadline);

 private:
  Connection* connection_;
  const std::string& query_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
