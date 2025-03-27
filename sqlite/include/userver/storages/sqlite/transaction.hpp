#pragma once

/// @file userver/storages/sqlite/transaction.hpp

#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/sqlite/impl/binder_help.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/savepoint.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Transaction final {
 public:
  Transaction(std::shared_ptr<infra::ConnectionPtr> connection,
              const settings::TransactionOptions& options);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;
  Transaction& operator=(Transaction&&) noexcept;

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query, const T& row) const;

  template <typename Container>
  void ExecuteMany(const Query& query, const Container& params) const;

  Savepoint Save(std::string name) const;

  void Commit();

  void Rollback();

 private:
  ResultSet DoExecute(impl::io::ParamsBinderBase& params) const;

  void AssertValid() const;

  std::shared_ptr<infra::ConnectionPtr> connection_;
};

template <typename... Args>
ResultSet Transaction::Execute(const Query& query, const Args&... args) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateParamsBindings(
      query.GetStatement(), *connection_, args...);
  return DoExecute(params_binder);
}

template <typename T>
ResultSet Transaction::ExecuteDecompose(const Query& query,
                                        const T& row) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
      query.GetStatement(), *connection_, row);
  return DoExecute(params_binder);
}

template <typename Container>
void Transaction::ExecuteMany(const Query& query,
                              const Container& params) const {
  AssertValid();
  for (const auto& row : params) {
    auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
        query.GetStatement(), *connection_, row);
    DoExecute(params_binder);
  }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
