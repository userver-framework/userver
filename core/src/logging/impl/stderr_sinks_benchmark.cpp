#include <benchmark/benchmark.h>

#include <spdlog/sinks/stdout_sinks.h>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/utils/rand.hpp>

#include "buffered_file_sink.hpp"
#include "fd_sink.hpp"

USERVER_NAMESPACE_BEGIN

constexpr ssize_t kCountLogs = 10;

void check_stderr_sink(benchmark::State& state) {
  auto sink = logging::impl::StderrSink();
  for (auto _ : state) {
    for (auto i = 0; i < kCountLogs; ++i) {
      sink.log({"default", spdlog::level::warn, "message"});
    }
  }
  sink.flush();
}
BENCHMARK(check_stderr_sink);

void check_stderr_file_sink(benchmark::State& state) {
  auto sink = logging::impl::BufferedStderrFileSink();
  for (auto _ : state) {
    for (auto i = 0; i < kCountLogs; ++i) {
      sink.log({"default", spdlog::level::warn, "message"});
    }
  }
  sink.flush();
}
BENCHMARK(check_stderr_file_sink);

void check_spdlog_stderr_sink(benchmark::State& state) {
  auto sink = spdlog::sinks::stderr_sink_mt();
  for (auto _ : state) {
    for (auto i = 0; i < kCountLogs; ++i) {
      sink.log({"default", spdlog::level::warn, "message"});
    }
  }
  sink.flush();
}
BENCHMARK(check_spdlog_stderr_sink);

USERVER_NAMESPACE_END
