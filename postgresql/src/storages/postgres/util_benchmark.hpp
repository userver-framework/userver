#pragma once

#include <benchmark/benchmark.h>

#include <functional>

#include <engine/task/task.hpp>
#include <storages/postgres/options.hpp>

void RunInCoro(std::function<void()>, size_t worker_threads = 1,
               std::optional<size_t> initial_coro_pool_size = {},
               std::optional<size_t> max_coro_pool_size = {});

namespace storages {
namespace postgres {
namespace bench {

constexpr static const char* kPostgresDsn = "POSTGRES_DSN_BENCH";
constexpr uint32_t kConnectionId = 0;

constexpr CommandControl kBenchCmdCtl{std::chrono::milliseconds{100},
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

}  // namespace bench
}  // namespace postgres
}  // namespace storages
