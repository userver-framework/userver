#include <userver/storages/sqlite/tests/utils.hpp>
#include <userver/utest/utest.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

UTEST(Query, InvalidStatement) {
  ConnectionWrapper connection{};

  UEXPECT_THROW(connection->Execute("SELECT * FROM this_table_doesnt_exist"),
                SQLiteStatementException);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
