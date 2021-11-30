#include <userver/storages/postgres/cluster.hpp>

#include <userver/utest/using_namespace_userver.hpp>

// We use wider lines to make the code look better on the landing page

// clang-format off

/// [Landing sample1]
std::size_t InsertKey(storages::postgres::ClusterPtr pg, std::string_view key) {
  // Asynchronous execution of the SQL query. Current thread handles other requests while the response from the DB is being received:
  auto res = pg->Execute(storages::postgres::ClusterHostType::kMaster, "INSERT INTO key_table (key) VALUES ($1)", key);

  return res.RowsAffected();
}
/// [Landing sample1]

// clang-format on
