#pragma once

#include <benchmark/benchmark.h>

#include <functional>

#include <engine/task/task.hpp>
#include <storages/postgres/postgres_fwd.hpp>

void RunInCoro(std::function<void()> user_cb, size_t worker_threads = 1);

namespace storages {
namespace postgres {
namespace bench {

constexpr static const char* kPostgresDsn = "POSTGRES_DSN_BENCH";
constexpr uint32_t kConnectionId = 0;

constexpr CommandControl kBenchCmdCtl{TimeoutDuration{100},
                                      TimeoutDuration{50}};

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
