#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>  // TODO: remove heavy include
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Transaction final {
 public:
  Transaction(infra::ConnectionPtr&& connection,
              const settings::TransactionOptions& options);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(settings::OptionalCommandControl option_cc,
                    const Query& query, const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query, const T& row) const;

  template <typename T>
  ResultSet ExecuteDecompose(settings::OptionalCommandControl optional_cc,
                             const Query& query, const T& row) const;

  template <typename Container>
  void ExecuteMany(const Query& query, const Container& params) const;

  template <typename Container>
  void ExecuteMany(settings::OptionalCommandControl optional_cc,
                   const Query& query, const Container& params) const;

  void Commit();

  void Rollback();

 private:
  template <typename... Args>
  ResultSet DoExecute(settings::OptionalCommandControl option_cc,
                      const Query& query, const Args&... args) const;

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Transaction::Execute(settings::OptionalCommandControl option_cc,
                               const Query& query, const Args&... args) const {
  return DoExecute(option_cc, query, args...);
}

template <typename T>
ResultSet Transaction::ExecuteDecompose(const Query& query,
                                        const T& row) const {
  return ExecuteDecompose(std::nullopt, query, row);
}

template <typename Container>
void Transaction::ExecuteMany(const Query& query,
                              const Container& params) const {
  return ExecuteMany(std::nullopt, query, params);
}

template <typename... Args>
ResultSet Transaction::DoExecute(settings::OptionalCommandControl option_cc,
                                 const Query& query,
                                 const Args&... args) const {
  AssertValid();
  return (*connection_)->ExecuteCommand(option_cc, query, args...);
}

template <typename T>
ResultSet Transaction::ExecuteDecompose(
    settings::OptionalCommandControl optional_cc, const Query& query,
    const T& row) const {
  AssertValid();
  return (*connection_)->ExecuteDecompose(optional_cc, query, row);
}

template <typename Container>
void Transaction::ExecuteMany(settings::OptionalCommandControl optional_cc,
                              const Query& query,
                              const Container& params) const {
  AssertValid();
  return (*connection_)->ExecuteMany(optional_cc, query, params);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
