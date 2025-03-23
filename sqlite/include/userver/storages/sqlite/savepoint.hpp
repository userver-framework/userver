#pragma once

/// @file userver/storages/sqlite/savepoint.hpp

#include <memory>

#include <userver/storages/sqlite/impl/binder_help.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Savepoint final {
 public:
  Savepoint(infra::ConnectionPtr&& connection, std::string name);
  ~Savepoint();
  Savepoint(const Savepoint& other) = delete;
  Savepoint(Savepoint&& other) noexcept;
  Savepoint& operator=(Savepoint&&) noexcept;

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query, const T& row) const;

  template <typename Container>
  void ExecuteMany(const Query& query, const Container& params) const;

  Savepoint Save(std::string name) const;

  void Release();

  void RollbackTo();

 private:
  Savepoint(std::shared_ptr<infra::ConnectionPtr> shared_connection,
            std::string name);  // for nested savepoints

  ResultSet DoExecute(impl::io::ParamsBinderBase& params) const;

  void AssertValid() const;

  friend class Transaction;

  std::shared_ptr<infra::ConnectionPtr> connection_;
  std::string name_;
};

template <typename... Args>
ResultSet Savepoint::Execute(const Query& query, const Args&... args) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateParamsBindings(
      query.GetStatement(), *connection_, args...);
  return DoExecute(params_binder);
}

template <typename T>
ResultSet Savepoint::ExecuteDecompose(const Query& query, const T& row) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
      query.GetStatement(), *connection_, row);
  return DoExecute(params_binder);
}

template <typename Container>
void Savepoint::ExecuteMany(const Query& query, const Container& params) const {
  AssertValid();
  for (const auto& row : params) {
    auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
        query.GetStatement(), *connection_, row);
    DoExecute(params_binder);
  }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
