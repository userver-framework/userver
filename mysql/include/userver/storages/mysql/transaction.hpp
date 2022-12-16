#pragma once

#include <userver/tracing/span.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/impl/bind_helper.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class ConnectionPtr;
}

class Transaction final {
 public:
  explicit Transaction(infra::ConnectionPtr&& connection,
                       engine::Deadline deadline);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;

  template <typename... Args>
  StatementResultSet Execute(const std::string& query,
                             const Args&... args) const;

  template <typename T>
  void InsertOne(const std::string& insert_query, const T& row) const;

  // only works with recent enough MariaDB as a server
  template <typename Container>
  void InsertMany(const std::string& insert_query, const Container& rows,
                  bool throw_on_empty_insert = true) const;

  void Commit();
  void Rollback();

 private:
  StatementResultSet DoExecute(const std::string& query,
                               impl::io::ParamsBinderBase& params) const;

  void DoInsert(const std::string& query,
                impl::io::ParamsBinderBase& params) const;

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
  engine::Deadline deadline_;
  tracing::Span span_;
};

template <typename... Args>
StatementResultSet Transaction::Execute(const std::string& query,
                                        const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return DoExecute(query, params_binder);
}

template <typename T>
void Transaction::InsertOne(const std::string& insert_query,
                            const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  return DoInsert(insert_query, params_binder);
}

template <typename Container>
void Transaction::InsertMany(const std::string& insert_query,
                             const Container& rows,
                             bool throw_on_empty_insert) const {
  if (rows.empty()) {
    if (throw_on_empty_insert) {
      throw std::runtime_error{"Empty insert requested"};
    } else {
      return;
    }
  }

  auto params_binder = impl::BindHelper::BindContainerAsParams(rows);

  DoInsert(insert_query, params_binder);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
