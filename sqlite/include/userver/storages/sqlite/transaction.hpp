#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/sqlite/impl/statements_base.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

namespace infra {
class ConnectionPtr;
}  // namespace infra

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
  ResultSet Execute(settings::OptionalCommandControl optional_cc,
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
  ResultSet DoExecute(settings::OptionalCommandControl optional_cc,
                      impl::StatementBasePtr prepare_statement) const;

  impl::StatementBasePtr PrepareStatement(const Query& query) const;

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Transaction::Execute(settings::OptionalCommandControl optional_cc,
                               const Query& query, const Args&... args) const {
  auto prepare_statement = PrepareStatement(query);
  prepare_statement->UpdateParamsBindings(args...);
  return DoExecute(optional_cc, prepare_statement);
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

template <typename T>
ResultSet Transaction::ExecuteDecompose(
    settings::OptionalCommandControl optional_cc, const Query& query,
    const T& row) const {
  AssertValid();
  auto prepare_statement = PrepareStatement(query);
  prepare_statement->UpdateRowAsParamsBindings(row);

  return DoExecute(optional_cc, prepare_statement);
}

template <typename Container>
void Transaction::ExecuteMany(settings::OptionalCommandControl optional_cc,
                              const Query& query,
                              const Container& params) const {
  AssertValid();
  for (const auto& row : params) {
    auto prepare_statement = PrepareStatement(query);
    prepare_statement->UpdateRowAsParamsBindings(row);
    DoExecute(optional_cc, prepare_statement);
  }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
