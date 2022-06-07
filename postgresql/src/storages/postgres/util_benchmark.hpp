#pragma once

#include <functional>

#include <benchmark/benchmark.h>

#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::bench {

inline constexpr const char* kPostgresDsn = "POSTGRES_DSN_BENCH";
inline constexpr uint32_t kConnectionId = 0;

inline constexpr CommandControl kBenchCmdCtl{std::chrono::milliseconds{100},
                                             std::chrono::milliseconds{50}};

class PgConnection : public benchmark::Fixture {
 protected:
  bool IsConnectionValid() const;

  detail::Connection& GetConnection() const noexcept { return *conn_; }

  // Should be used for starting the benchmark's coroutine environment instead
  // of engine::RunStandalone
  void RunStandalone(benchmark::State& state, std::function<void()> payload);

  void RunStandalone(benchmark::State& state, std::size_t thread_count,
                     std::function<void()> payload);

 private:
  std::unique_ptr<detail::Connection> conn_;
};

}  // namespace storages::postgres::bench

USERVER_NAMESPACE_END
