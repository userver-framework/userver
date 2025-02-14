#pragma once

#include <memory>

#include <sqlite3.h>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statements.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ConnectionImpl {
 public:
  ConnectionImpl(const SQLiteSettings& settings,
                 engine::TaskProcessor& blocking_task_processor);

  ConnectionSettings const& GetSettings() const noexcept;
  sqlite3* GetHandle() const noexcept;

  template <typename... Args>
  ResultSet ExecuteCommand(OptionalCommandControl optional_cc,
                           const Query& query, const Args&... args);

  template <typename T>
  ResultSet ExecuteDecompose(OptionalCommandControl optional_cc,
                             const Query& query, const T& row);

  template <typename Container>
  void ExecuteMany(OptionalCommandControl optional_cc, const Query& query,
                   const Container& params);

  void Begin(const TransactionOptions& options);

  void Commit();

  void Rollback();

  void Savepoint(const std::string& name);

  void Release(const std::string& name);

  void RollbackTo(const std::string& name);

  std::string PrepareString(const std::string& str);

 private:
  struct SQLiteHandlerDeleter {
    void operator()(sqlite3* sqlite_handle);
  };

  using NativeHandlerPtr = std::unique_ptr<sqlite3, SQLiteHandlerDeleter>;

  sqlite3* OpenDatabase(const SQLiteSettings& settings) const;

  std::shared_ptr<Statement> MakeStatement(const std::string& statement) const;

  template <typename... Args>
  ResultSet ExecuteCommand(const Query& query, const Args&... args);

  template <typename... Args>
  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    const Args&... args) const;

  engine::TaskProcessor& blocking_task_processor_;
  ConnectionSettings settings_;
  NativeHandlerPtr db_handler_;
  impl::StatementsCache statements_cache_;
};

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommand(OptionalCommandControl optional_cc
                                         [[maybe_unused]],
                                         const Query& query,
                                         const Args&... args) {
  // TODO Process optional_cc
  if (settings_.prepared_statements ==
      ConnectionSettings::kNoPreparedStatements) {
    return ExecuteCommandNoPrepare(query, args...);
  }
  return ExecuteCommand(query, args...);
}

template <typename T>
ResultSet ConnectionImpl::ExecuteDecompose(OptionalCommandControl optional_cc,
                                           const Query& query, const T& row) {
  // TODO: Add more detailed verification and error description
  static_assert(std::is_aggregate_v<T> || boost::pfr::tuple_size_v<T> > 0,
                "T must be an aggregate type or tuple-like type");
  if constexpr (std::is_aggregate_v<T>) {
    auto fields = boost::pfr::structure_to_tuple(row);
    return std::apply(
        [this, &query, &optional_cc](const auto&... args) {
          return this->ExecuteCommand(optional_cc, query, args...);
        },
        fields);
  } else {
    return std::apply(
        [this, &query, &optional_cc](const auto&... args) {
          return this->ExecuteCommand(optional_cc, query, args...);
        },
        row);
  }
}

template <typename Container>
void ConnectionImpl::ExecuteMany(OptionalCommandControl optional_cc,
                                 const Query& query, const Container& params) {
  for (const auto& row : params) {
    ExecuteDecompose(optional_cc, query, row);
  }
}

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommand(const Query& query,
                                         const Args&... args) {
  // Prepare statement and execute first step
  // TODO: For simple INSERT, DELETE, UPDATE this works, but for example using
  // RETURNING clauses, obviously repeated calls to sqlite3_step are required to
  // get all rows https://www.sqlite.org/lang_returning.html
  // Based on circumstantial evidence, nested and complex DML queries execute in
  // one sqlite3_step, but this requires inspection and profiling
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [this, query, args...] {
               auto stmt =
                   statements_cache_.PrepareStatement(query.GetStatement());
               stmt->Reset();
               stmt->UpdateParamsBindings(args...);

               stmt->Next();
               stmt->CheckFail();
               return ResultSet{std::make_shared<impl::ResultWrapper>(stmt)};
             })
      .Get();
}

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommandNoPrepare(const Query& query,
                                                  const Args&... args) const {
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [this, query, args...] {
               auto stmt = std::make_shared<impl::Statement>(
                   db_handler_.get(), query.GetStatement());
               stmt->UpdateParamsBindings(args...);

               stmt->Next();
               stmt->CheckFail();
               return ResultSet{std::make_shared<impl::ResultWrapper>(stmt)};
             })
      .Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
