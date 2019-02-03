#include <benchmark/benchmark.h>

#include <chrono>

#include <cctz/civil_time.h>
#include <cctz/time_zone.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/io/force_text.hpp>
#include <storages/postgres/io/user_types.hpp>
#include <storages/postgres/tests/test_buffers.hpp>

#include <storages/postgres/util_benchmark.hpp>

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

void PgTimestampTextFormat(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;

  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  for (auto _ : state) {
    io::WriteBuffer<io::DataFormat::kTextDataFormat>(types, buffer, tp);
    buffer.clear();
  }
}

void PgTimestampBinaryFormat(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;

  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  for (auto _ : state) {
    io::WriteBuffer<io::DataFormat::kBinaryDataFormat>(types, buffer, tp);
    buffer.clear();
  }
}

void PgTimestampTextParse(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;
  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  io::WriteBuffer<io::DataFormat::kTextDataFormat>(types, buffer, tp);
  auto fp = pg::test::MakeFieldBuffer(buffer, io::DataFormat::kTextDataFormat);
  for (auto _ : state) {
    io::ReadBuffer<io::DataFormat::kTextDataFormat>(fp, tp);
  }
}

void PgTimestampBinaryParse(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;
  auto tp = std::chrono::system_clock::now();
  pg::test::Buffer buffer;
  io::WriteBuffer<io::DataFormat::kBinaryDataFormat>(types, buffer, tp);
  auto fp =
      pg::test::MakeFieldBuffer(buffer, io::DataFormat::kBinaryDataFormat);
  for (auto _ : state) {
    io::ReadBuffer<io::DataFormat::kBinaryDataFormat>(fp, tp);
  }
}

BENCHMARK(CctzTimestampFormat);
BENCHMARK(CctzTimestampParse);
BENCHMARK(PgTimestampTextFormat);
BENCHMARK(PgTimestampBinaryFormat);
BENCHMARK(PgTimestampTextParse);
BENCHMARK(PgTimestampBinaryParse);

BENCHMARK_F(PgConnection, TimestampBinaryRoundtrip)(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;
  auto connected = IsConnectionValid();
  if (!connected) {
    state.SkipWithError("Database not connected");
  } else {
    RunInCoro([this, &state] {
      auto tp = std::chrono::system_clock::now();
      for (auto _ : state) {
        auto res = conn_->ExperimentalExecute(
            "select $1", io::DataFormat::kBinaryDataFormat, tp);
        res.Front().To(tp);
      }
    });
  }
}

BENCHMARK_F(PgConnection, TimestampTextRoundtrip)(benchmark::State& state) {
  namespace pg = storages::postgres;
  namespace io = pg::io;
  auto connected = IsConnectionValid();
  if (!connected) {
    state.SkipWithError("Database not connected");
  } else {
    RunInCoro([this, &state] {
      auto tp = std::chrono::system_clock::now();
      for (auto _ : state) {
        auto res = conn_->ExperimentalExecute("select $1",
                                              io::DataFormat::kTextDataFormat,
                                              pg::ForceTextFormat(tp));
        res.Front().To(tp);
      }
    });
  }
}

}  // namespace
