#include <storages/postgres/util_benchmark.hpp>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/task/task.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/storages/postgres/dsn.hpp>

USERVER_NAMESPACE_BEGIN

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

void PgConnection::RunStandalone(benchmark::State& state,
                                 std::function<void()> payload) {
  RunStandalone(state, 1, std::move(payload));
}

void PgConnection::RunStandalone(benchmark::State& state,
                                 std::size_t thread_count,
                                 std::function<void()> payload) {
  engine::RunStandalone(thread_count, [&] {
    auto bts = concurrent::BackgroundTaskStorageCore{};
    auto dsn = GetDsnFromEnv();
    if (!dsn.empty()) {
      conn_ = detail::Connection::Connect(
          dsn, nullptr, engine::current_task::GetTaskProcessor(), bts,
          kConnectionId, {ConnectionSettings::kCachePreparedStatements},
          DefaultCommandControls(kBenchCmdCtl, {}, {}), {}, {});
    }

    if (!IsConnectionValid()) {
      state.SkipWithError("Database not connected");
      return;
    }

    payload();

    conn_->Close();
    conn_.reset();
  });
}

bool PgConnection::IsConnectionValid() const {
  return conn_ && conn_->IsConnected();
}

}  // namespace storages::postgres::bench

USERVER_NAMESPACE_END
