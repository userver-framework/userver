#pragma once

#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/io/binder.hpp>
#include <userver/storages/mysql/io/insert_binder.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class ConnectionPtr;
}

class Transaction final {
 public:
  explicit Transaction(infra::ConnectionPtr&& connection);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;

  template <typename... Args>
  StatementResultSet Execute(engine::Deadline deadline,
                             const std::string& query, const Args&... args);

  template <typename T>
  void InsertOne(engine::Deadline deadline, const std::string& insert_query,
                 const T& row) const;

  // only works with recent enough MariaDB as a server
  template <typename Container>
  void InsertMany(engine::Deadline deadline, const std::string& insert_query,
                  const Container& rows,
                  bool throw_on_empty_insert = true) const;

  void Commit(engine::Deadline deadline);
  void Rollback(engine::Deadline deadline);

 private:
  StatementResultSet DoExecute(const std::string& query,
                               io::ParamsBinderBase& params,
                               engine::Deadline deadline);

  void DoInsert(const std::string& query, io::ParamsBinderBase& params,
                engine::Deadline deadline);

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
};

template <typename... Args>
StatementResultSet Transaction::Execute(engine::Deadline deadline,
                                        const std::string& query,
                                        const Args&... args) {
  auto params_binder = io::ParamsBinder::BindParams(args...);

  return DoExecute(query, params_binder, deadline);
}

template <typename T>
void Transaction::InsertOne(engine::Deadline deadline,
                            const std::string& insert_query,
                            const T& row) const {
  using Row = std::decay_t<T>;
  static_assert(boost::pfr::tuple_size_v<Row> != 0,
                "Row to insert has zero columns");

  auto binder = std::apply(
      [](const auto&... args) {
        return io::ParamsBinder::BindParams(std::forward(args)...);
      },
      boost::pfr::structure_tie(row));

  return DoInsert(insert_query, binder, deadline);
}

template <typename Container>
void Transaction::InsertMany(engine::Deadline deadline,
                             const std::string& insert_query,
                             const Container& rows,
                             bool throw_on_empty_insert) const {
  if (rows.empty()) {
    if (throw_on_empty_insert) {
      throw std::runtime_error{"Empty insert requested"};
    } else {
      return;
    }
  }

  io::InsertBinder binder{rows};
  binder.BindRows();

  DoInsert(insert_query, binder, deadline);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
