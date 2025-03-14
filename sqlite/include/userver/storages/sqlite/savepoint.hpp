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
  Savepoint(std::shared_ptr<infra::ConnectionPtr> connection, std::string name);
  ~Savepoint();
  Savepoint(const Savepoint& other) = delete;
  Savepoint(Savepoint&& other) noexcept;
  Savepoint& operator=(Savepoint&&) noexcept;

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

  void Release();

  void RollbackTo();

  Savepoint Save(std::string name);

 private:
  ResultSet DoExecute(settings::OptionalCommandControl optional_cc,
                      impl::io::ParamsBinderBase& params) const;

  impl::StatementBasePtr PrepareStatement(const Query& query) const;

  void AssertValid() const;

  std::shared_ptr<infra::ConnectionPtr> connection_;
  std::string name_;
};

template <typename... Args>
ResultSet Savepoint::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Savepoint::Execute(settings::OptionalCommandControl optional_cc,
                             const Query& query, const Args&... args) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateParamsBindings(
      query.GetStatement(), *connection_, args...);
  return DoExecute(optional_cc, params_binder);
}

template <typename T>
ResultSet Savepoint::ExecuteDecompose(const Query& query, const T& row) const {
  return ExecuteDecompose(std::nullopt, query, row);
}

template <typename T>
ResultSet Savepoint::ExecuteDecompose(
    settings::OptionalCommandControl optional_cc, const Query& query,
    const T& row) const {
  AssertValid();
  auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
      query.GetStatement(), *connection_, row);
  return DoExecute(optional_cc, params_binder);
}

template <typename Container>
void Savepoint::ExecuteMany(const Query& query, const Container& params) const {
  return ExecuteMany(std::nullopt, query, params);
}

template <typename Container>
void Savepoint::ExecuteMany(settings::OptionalCommandControl optional_cc,
                            const Query& query, const Container& params) const {
  AssertValid();
  for (const auto& row : params) {
    auto params_binder = impl::BindHelper::UpdateRowAsParamsBindings(
        query.GetStatement(), *connection_, row);
    DoExecute(optional_cc, params_binder);
  }
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
