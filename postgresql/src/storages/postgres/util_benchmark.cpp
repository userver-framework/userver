#include <storages/postgres/util_benchmark.hpp>

#include <engine/standalone.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>

namespace storages::postgres::bench {

namespace {

Dsn GetDsnFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return Dsn{{}};
  }
  auto by_host = SplitByHost(Dsn{conn_list_env});
  if (by_host.empty()) {
    return Dsn{{}};
  }
  return by_host[0];
}

}  // namespace

PgConnection::PgConnection() = default;

PgConnection::~PgConnection() = default;

void PgConnection::SetUp(benchmark::State&) {
  auto dsn = GetDsnFromEnv();
  if (!dsn.GetUnprotectedRawValue().empty()) {
    RunInCoro([this, dsn] {
      conn_ = detail::Connection::Connect(
          dsn, GetTaskProcessor(), kConnectionId,
          {ConnectionSettings::kCachePreparedStatements}, kBenchCmdCtl, {}, {});
    });
  }
}

void PgConnection::TearDown(benchmark::State&) {
  if (IsConnectionValid()) {
    RunInCoro([this] { conn_->Close(); });
  }
}

bool PgConnection::IsConnectionValid() const {
  return conn_ && conn_->IsConnected();
}

engine::TaskProcessor& PgConnection::GetTaskProcessor() {
  return engine::current_task::GetCurrentTaskContext()->GetTaskProcessor();
}

}  // namespace storages::postgres::bench
