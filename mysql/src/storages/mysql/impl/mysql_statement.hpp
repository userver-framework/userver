#pragma once

#include <memory>
#include <string>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {
class ExtractorBase;
}

namespace storages::mysql::impl {

class MySQLConnection;
class BindsStorage;

class MySQLStatement;

class MySQLStatementFetcher final {
 public:
  ~MySQLStatementFetcher();

  MySQLStatementFetcher(const MySQLStatementFetcher& other) = delete;
  MySQLStatementFetcher(MySQLStatementFetcher&& other) noexcept;

  void FetchResult(io::ExtractorBase& extractor, engine::Deadline deadline);

 private:
  friend class MySQLStatement;
  MySQLStatementFetcher(MySQLStatement& statement);

  void ValidateBinding(BindsStorage& binds);

  MySQLStatement& statement_;
};

class MySQLStatement final {
 public:
  MySQLStatement(MySQLConnection& connection, const std::string& statement,
                 engine::Deadline deadline);
  ~MySQLStatement();

  MySQLStatement(const MySQLStatement& other) = delete;
  MySQLStatement(MySQLStatement&& other) noexcept;

  MySQLStatementFetcher Execute(engine::Deadline deadline, BindsStorage& binds);

  void SetInsertRowsCount(std::size_t rows_count);

  std::size_t RowsCount() const;

  std::size_t ParamsCount();
  std::size_t ResultColumnsCount();

 private:
  friend class MySQLStatementFetcher;
  void StoreResult(engine::Deadline deadline);
  bool FetchResultRow(BindsStorage& binds);
  void FreeResult(engine::Deadline deadline);

  void UpdateParamsBindings(BindsStorage& binds);

  class NativeStatementDeleter final {
   public:
    NativeStatementDeleter(MySQLConnection* connection);

    void operator()(MYSQL_STMT* statement);

   private:
    MySQLConnection* connection_;
  };
  using NativeStatementPtr =
      std::unique_ptr<MYSQL_STMT, NativeStatementDeleter>;

  NativeStatementPtr CreateStatement(const std::string& statement,
                                     engine::Deadline deadline);
  std::string GetNativeError(std::string_view prefix) const;

  MySQLConnection* connection_;
  NativeStatementPtr native_statement_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
