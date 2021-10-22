#pragma once

#include <benchmark/benchmark.h>

#include <functional>

#include <userver/engine/run_in_coro.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::bench {

inline constexpr const char* kPostgresDsn = "POSTGRES_DSN_BENCH";
inline constexpr uint32_t kConnectionId = 0;

inline constexpr CommandControl kBenchCmdCtl{std::chrono::milliseconds{100},
                                             std::chrono::milliseconds{50}};

class PgConnection : public benchmark::Fixture {
 public:
  PgConnection();
  ~PgConnection();

 protected:
  void SetUp(benchmark::State& st) override;
  void TearDown(benchmark::State& st) override;

  bool IsConnectionValid() const;

  static engine::TaskProcessor& GetTaskProcessor();

  std::unique_ptr<detail::Connection> conn_;
};

}  // namespace storages::postgres::bench

USERVER_NAMESPACE_END
