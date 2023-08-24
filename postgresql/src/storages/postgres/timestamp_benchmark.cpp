#include <benchmark/benchmark.h>

#include <chrono>

#include <cctz/civil_time.h>
#include <cctz/time_zone.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/tests/test_buffers.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <storages/postgres/util_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;
using namespace pg::bench;

const std::string time_format = "%Y-%m-%d %H:%M:%E*s%Ez";
const pg::UserTypes types;

void CctzTimestampFormat(benchmark::State& state) {
  const auto utc = cctz::utc_time_zone();
  auto tp = std::chrono::system_clock::now();
  for (auto _ : state) {
    cctz::format(time_format, tp, utc);
  }
}

void CctzTimestampParse(benchmark::State& state) {
  const std::string pg_timestamp = "2018-11-07 13:28:44.194045+03";
  const auto utc = cctz::utc_time_zone();
  std::chrono::system_clock::time_point tp;
  for (auto _ : state) {
    cctz::parse(time_format, pg_timestamp, utc, &tp);
  }
}

void PgTimestampBinaryFormat(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;

  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  for (auto _ : state) {
    io::WriteBuffer(types, buffer, tp);
    buffer.clear();
  }
}

void PgTimestampBinaryParse(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;
  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  io::WriteBuffer(types, buffer, tp);
  auto fp = pg::test::MakeFieldBuffer(buffer);
  for (auto _ : state) {
    io::ReadBuffer(fp, tp);
  }
}

BENCHMARK(CctzTimestampFormat);
BENCHMARK(CctzTimestampParse);
BENCHMARK(PgTimestampBinaryFormat);
BENCHMARK(PgTimestampBinaryParse);

BENCHMARK_F(PgConnection, TimestampBinaryRoundtrip)(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;

  RunStandalone(state, [this, &state] {
    auto tp = std::chrono::system_clock::now();
    for (auto _ : state) {
      auto res = GetConnection().Execute("select $1", tp);
      res.Front().To(tp);
    }
  });
}

}  // namespace

USERVER_NAMESPACE_END
