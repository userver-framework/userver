#pragma once

#include <memory>
#include <optional>
#include <string>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace io {
class ExtractorBase;
class ParamsBinderBase;
}  // namespace io

class Connection;

namespace bindings {
class OutputBindings;
}

class BrokenGuard;
class Statement;

class StatementFetcher final {
 public:
  ~StatementFetcher();

  StatementFetcher(const StatementFetcher& other) = delete;
  StatementFetcher(StatementFetcher&& other) noexcept;

  bool FetchResult(io::ExtractorBase& extractor);

  std::uint64_t RowsAffected() const;

  std::uint64_t LastInsertId() const;

 private:
  friend class Statement;
  explicit StatementFetcher(Statement& statement);

  engine::Deadline parent_statement_deadline_;
  bool binds_applied_{false};
  bool binds_validated_{false};
  Statement* statement_;
};

class Statement final {
 public:
  Statement(Connection& connection, const std::string& statement,
            engine::Deadline deadline);
  ~Statement();

  Statement(const Statement& other) = delete;
  Statement(Statement&& other) noexcept;

  StatementFetcher Execute(io::ParamsBinderBase& params,
                           engine::Deadline deadline);

  std::size_t RowsCount() const;

  std::size_t ParamsCount();
  std::size_t ResultColumnsCount();

  void SetReadonlyCursor(std::size_t batch_size);
  void SetNoCursor();
  std::optional<std::size_t> GetBatchSize() const;

  void SetDestructionDeadline(engine::Deadline deadline);

  const std::string& GetStatementText() const;

 private:
  friend class StatementFetcher;
  void StoreResult(engine::Deadline deadline);

  bool FetchResultRow(bindings::OutputBindings& binds, bool apply_binds,
                      engine::Deadline deadline);
  void Reset(engine::Deadline deadline);

  void UpdateParamsBindings(io::ParamsBinderBase& params);

  class NativeStatementDeleter {
   public:
    explicit NativeStatementDeleter(Connection* connection);

    void operator()(MYSQL_STMT* statement) const noexcept;

    void SetDeadline(engine::Deadline deadline);

   private:
    Connection* connection_;
    engine::Deadline deadline_;
  };
  using NativeStatementPtr =
      std::unique_ptr<MYSQL_STMT, NativeStatementDeleter>;

  NativeStatementPtr CreateStatement(engine::Deadline deadline);
  void PrepareStatement(NativeStatementPtr& native_statement,
                        engine::Deadline deadline);
  std::string GetNativeError(std::string_view prefix) const;

  std::string statement_;

  Connection* connection_;
  NativeStatementPtr native_statement_;

  std::optional<std::size_t> batch_size_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
