#pragma once

#include <memory>
#include <optional>
#include <string>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {
class ExtractorBase;
class ParamsBinderBase;
}  // namespace storages::mysql::io

namespace storages::mysql::impl {

class MySQLConnection;
namespace bindings {
class OutputBindings;
}

class MySQLStatement;

class MySQLStatementFetcher final {
 public:
  ~MySQLStatementFetcher();

  MySQLStatementFetcher(const MySQLStatementFetcher& other) = delete;
  MySQLStatementFetcher(MySQLStatementFetcher&& other) noexcept;

  bool FetchResult(io::ExtractorBase& extractor);

 private:
  friend class MySQLStatement;
  MySQLStatementFetcher(MySQLStatement& statement);

  engine::Deadline parent_statement_deadline_;
  bool binds_validated_{false};
  MySQLStatement* statement_;
};

class MySQLStatement final {
 public:
  MySQLStatement(MySQLConnection& connection, const std::string& statement,
                 engine::Deadline deadline);
  ~MySQLStatement();

  MySQLStatement(const MySQLStatement& other) = delete;
  MySQLStatement(MySQLStatement&& other) noexcept;

  MySQLStatementFetcher Execute(engine::Deadline deadline,
                                io::ParamsBinderBase& params);

  std::size_t RowsCount() const;

  std::size_t ParamsCount();
  std::size_t ResultColumnsCount();

  void SetReadonlyCursor(std::size_t batch_size);
  void SetNoCursor();
  std::optional<std::size_t> GetBatchSize() const;

 private:
  friend class MySQLStatementFetcher;
  void StoreResult(engine::Deadline deadline);

  bool FetchResultRow(bindings::OutputBindings& binds,
                      engine::Deadline deadline);
  void Reset(engine::Deadline deadline);

  void UpdateParamsBindings(io::ParamsBinderBase& params);

  class NativeStatementDeleter final {
   public:
    NativeStatementDeleter(MySQLConnection* connection);

    void operator()(MYSQL_STMT* statement);

   private:
    MySQLConnection* connection_;
  };
  using NativeStatementPtr =
      std::unique_ptr<MYSQL_STMT, NativeStatementDeleter>;

  NativeStatementPtr CreateStatement(engine::Deadline deadline);
  void PrepareStatement(NativeStatementPtr& native_statement,
                        engine::Deadline deadline);
  std::string GetNativeError(std::string_view prefix) const;

  std::string statement_;

  MySQLConnection* connection_;
  NativeStatementPtr native_statement_;

  std::optional<std::size_t> batch_size_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
