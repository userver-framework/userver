#pragma once

#include <memory>
#include <string>

#include <mysql/mysql.h>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLConnection;
class BindsStorage;

class MySQLStatement final {
 public:
  MySQLStatement(MySQLConnection& connection, const std::string& statement,
                 engine::Deadline deadline);
  ~MySQLStatement();

  MySQLStatement(const MySQLStatement& other) = delete;
  MySQLStatement(MySQLStatement&& other) noexcept;

  void BindStatementParams(BindsStorage& binds);
  void Execute(engine::Deadline deadline);

  void StoreResult(engine::Deadline deadline);
  bool FetchResultRow(BindsStorage& binds);

  void FreeResult(engine::Deadline deadline);

  std::size_t RowsCount() const;

  std::size_t ParamsCount();
  std::size_t ResultColumnsCount();

 private:
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

  MySQLConnection* connection_;
  NativeStatementPtr native_statement_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
