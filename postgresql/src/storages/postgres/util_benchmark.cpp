#include <storages/postgres/util_benchmark.hpp>

#include <engine/standalone.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {
namespace bench {

namespace {

std::string GetDsnFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return {};
  }
  auto by_host = SplitByHost(conn_list_env);
  if (by_host.empty()) {
    return {};
  }
  return by_host[0];
}

}  // namespace

PgConnection::PgConnection() = default;

PgConnection::~PgConnection() = default;

void PgConnection::SetUp(benchmark::State&) {
  auto conninfo = GetDsnFromEnv();
  if (!conninfo.empty()) {
    RunInCoro([this, conninfo] {
      conn_ = detail::Connection::Connect(conninfo, GetTaskProcessor(),
                                          kConnectionId, kBenchCmdCtl);
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

}  // namespace bench
}  // namespace postgres
}  // namespace storages
