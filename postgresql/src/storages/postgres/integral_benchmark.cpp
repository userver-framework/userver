#include <benchmark/benchmark.h>

#include <limits>

#include <storages/postgres/detail/connection.hpp>

#include <storages/postgres/util_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;
using namespace pg::bench;

BENCHMARK_F(PgConnection, BoolRoundtrip)(benchmark::State& state) {
  RunStandalone(state, [this, &state] {
    bool v = true;
    for (auto _ : state) {
      auto res = GetConnection().Execute("select $1", v);
      res.Front().To(v);
    }
  });
}

BENCHMARK_F(PgConnection, Int16Roundtrip)(benchmark::State& state) {
  RunStandalone(state, [this, &state] {
    std::int16_t v = std::numeric_limits<std::int16_t>::max();
    for (auto _ : state) {
      auto res = GetConnection().Execute("select $1", v);
      res.Front().To(v);
    }
  });
}

BENCHMARK_F(PgConnection, Int32Roundtrip)(benchmark::State& state) {
  RunStandalone(state, [this, &state] {
    std::int32_t v = std::numeric_limits<std::int32_t>::max();
    for (auto _ : state) {
      auto res = GetConnection().Execute("select $1", v);
      res.Front().To(v);
    }
  });
}

BENCHMARK_F(PgConnection, Int64Roundtrip)(benchmark::State& state) {
  RunStandalone(state, [this, &state] {
    std::int64_t v = std::numeric_limits<std::int64_t>::max();
    for (auto _ : state) {
      auto res = GetConnection().Execute("select $1", v);
      res.Front().To(v);
    }
  });
}

}  // namespace

USERVER_NAMESPACE_END
