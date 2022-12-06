#include <userver/utest/utest.hpp>

#include <iostream>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/pool.hpp>

#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Connection, Works) { storages::mysql::impl::MySQLConnection conn{}; }

UTEST(Connection, ExecuteWorks) {
  storages::mysql::impl::MySQLConnection conn{};

  const auto res = conn.Execute("SELECT Id, Value FROM test", {});

  for (const auto& row : res) {
    for (const auto& field : row) {
      std::cout << field << "; ";
    }
    std::cout << std::endl;
  }
}

UTEST(Pool, Works) { const auto pool = storages::mysql::infra::Pool::Create(); }

USERVER_NAMESPACE_END
