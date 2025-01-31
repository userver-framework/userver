#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <memory>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/engine/async.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include "userver/storages/sqlite/impl/connection_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace infra {
class ConnectionPtr;
}

class Transaction final {
 public:
  Transaction(std::shared_ptr<impl::ConnectionImpl> pimpl,
              const TransactionOptions& options);
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
  std::shared_ptr<impl::ConnectionImpl> pimpl_;

  template <typename... Args>
  ResultSet DoExecute(OptionalCommandControl option_cc [[maybe_unused]],
                      const Query& query,
                      const Args&... args [[maybe_unused]]) const;
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
  return DoExecute(option_cc, query, args...);
}

template <typename... Args>
ResultSet Transaction::DoExecute(OptionalCommandControl option_cc
                                 [[maybe_unused]],
                                 const Query& query,
                                 const Args&... args [[maybe_unused]]) const {
  return pimpl_->ExecuteCommand(option_cc, query, args...);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
