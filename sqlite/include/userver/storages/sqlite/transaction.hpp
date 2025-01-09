#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace infra {
class ConnectionPtr;
}

class Transaction final {
 public:
  Transaction(sqlite3* handle, engine::TaskProcessor& blocking_task_processor);
  ~Transaction();
  Transaction(const Transaction& other);
  Transaction(Transaction&& other) noexcept;

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(OptionalCommandControl option_cc, const Query& query,
                    const Args&... args) const;

  // TODO: need more diffrent Execute?

  // TODO: need Portal?

  void Commit();

  void Rollback();

 private:
  sqlite3* handle_ = nullptr;  // TODO: it's stub
  engine::TaskProcessor& blocking_task_processor_;

  ResultSet DoExecute(const Query& query) const;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
  return Execute(OptionalCommandControl{}, query, args...);
}

template <typename... Args>
ResultSet Transaction::Execute(OptionalCommandControl option_cc
                               [[maybe_unused]],
                               const Query& query,
                               const Args&... args [[maybe_unused]]) const {
  return DoExecute(query);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
