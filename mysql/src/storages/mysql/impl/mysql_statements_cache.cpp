#include <storages/mysql/impl/mysql_statements_cache.hpp>

#include <userver/tracing/scope_time.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLStatementsCache::MySQLStatementsCache(MySQLConnection& connection,
                                           std::size_t capacity)
    : connection_{connection}, capacity_{capacity} {
  UASSERT(capacity_ > 0);
}

MySQLStatementsCache::~MySQLStatementsCache() = default;

MySQLStatement& MySQLStatementsCache::PrepareStatement(
    const std::string& statement, engine::Deadline deadline) {
  auto it = lookup_.find(statement);
  if (it == lookup_.end()) {
    {
      tracing::ScopeTime prepare{"prepare"};
      queue_.emplace_front(
          std::piecewise_construct, std::forward_as_tuple(statement),
          std::forward_as_tuple(connection_, statement, deadline));
      it = lookup_.emplace(statement, queue_.begin()).first;
    }

    if (queue_.size() > capacity_) {
      tracing::ScopeTime clear_lru{"reset_old_statements"};
      lookup_.erase(queue_.back().first);
      queue_.pop_back();
    }
  } else {
    queue_.splice(queue_.begin(), queue_, it->second);
    it->second = queue_.begin();
  }

  return it->second->second;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
