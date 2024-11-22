#include <userver/utest/utest.hpp>

#include <userver/storages/sqlite.hpp>
#include <userver/storages/sqlite/tests/utils.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::tests {

// Here we want to check the operation of Query, probably the calculation check should not be done here

UTEST(Query, InvalidStatement) {
  ConnectionWrapper connection{};

  UEXPECT_THROW(connection->Execute("SELECT * FROM this_table_doesnt_exist"),
                SQLiteStatementException);
}

}  // namespace storages::sqlite::tests

USERVER_NAMESPACE_END
