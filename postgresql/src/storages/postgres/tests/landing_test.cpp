#include <userver/storages/postgres/cluster.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [Landing sample1]
std::size_t Ins(storages::postgres::Transaction& tr, std::string_view key) {
  // Asynchronous execution of the SQL query in transaction. Current thread
  // handles other requests while the response from the DB is being received:
  auto res = tr.Execute("INSERT INTO keys VALUES ($1)", key);
  return res.RowsAffected();
}
/// [Landing sample1]

/// [Exec sample]
std::size_t InsertSomeKey(storages::postgres::Cluster& pg,
                          std::string_view key) {
  auto res = pg.Execute(storages::postgres::ClusterHostType::kMaster,
                        "INSERT INTO keys VALUES ($1)", key);
  return res.RowsAffected();
}
/// [Exec sample]

/// [TransacExec]
std::size_t InsertInTransaction(storages::postgres::Transaction& transaction,
                                std::string_view key) {
  auto res = transaction.Execute("INSERT INTO keys VALUES ($1)", key);
  return res.RowsAffected();
}
/// [TransacExec]
