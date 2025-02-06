#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <memory>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite/impl/connection_impl.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Transaction final {
 public:
  Transaction(std::shared_ptr<impl::ConnectionImpl> pimpl,
              const TransactionOptions& options);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(OptionalCommandControl option_cc, const Query& query,
                    const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query, const T& row) const;

  template <typename T>
  ResultSet ExecuteDecompose(OptionalCommandControl optional_cc,
                             const Query& query, const T& row) const;

  template <typename Container>
  void ExecuteMany(const Query& query, const Container& params) const;

  template <typename Container>
  void ExecuteMany(OptionalCommandControl optional_cc, const Query& query,
                   const Container& params) const;

  // TODO: need Portal?

  void Commit();

  void Rollback();

 private:
  template <typename... Args>
  ResultSet DoExecute(OptionalCommandControl option_cc, const Query& query,
                      const Args&... args) const;

  // TODO: maybe it is better to use a class of an index for implementation or
  // fastpimpl
  std::shared_ptr<impl::ConnectionImpl> pimpl_;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Transaction::Execute(OptionalCommandControl option_cc,
                               const Query& query, const Args&... args) const {
  return DoExecute(option_cc, query, args...);
}

template <typename... Args>
ResultSet Transaction::DoExecute(OptionalCommandControl option_cc,
                                 const Query& query,
                                 const Args&... args) const {
  return pimpl_->ExecuteCommand(option_cc, query, args...);
}

template <typename T>
ResultSet Transaction::ExecuteDecompose(const Query& query,
                                        const T& row) const {
  return ExecuteDecompose(std::nullopt, query, row);
}

template <typename T>
ResultSet Transaction::ExecuteDecompose(OptionalCommandControl optional_cc,
                                        const Query& query,
                                        const T& row) const {
  return pimpl_->ExecuteDecompose(optional_cc, query, row);
}

template <typename Container>
void Transaction::ExecuteMany(const Query& query,
                              const Container& params) const {
  return ExecuteMany(std::nullopt, query, params);
}

template <typename Container>
void Transaction::ExecuteMany(OptionalCommandControl optional_cc,
                              const Query& query,
                              const Container& params) const {
  return pimpl_->ExecuteMany(optional_cc, query, params);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
