#include <benchmark/benchmark.h>

#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/utils/rand.hpp>

#include "buffered_file_sink.hpp"
#include "file_sink.hpp"

USERVER_NAMESPACE_BEGIN

constexpr ssize_t kCountLogs = 100000;

void check_file_sink(benchmark::State& state) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string filename =
      temp_root.GetPath() + "/temp_file_" + std::to_string(utils::Rand());
  auto sink = logging::impl::FileSink(filename);
  for ([[maybe_unused]] auto _ : state) {
    for (auto i = 0; i < kCountLogs; ++i) {
      sink.Log({"message\n", logging::Level::kWarning});
    }
  }
  sink.Flush();
}
BENCHMARK(check_file_sink);

void check_buffered_file_sink(benchmark::State& state) {
  const auto temp_root = fs::blocking::TempDirectory::Create();
  const std::string filename =
      temp_root.GetPath() + "/temp_file_" + std::to_string(utils::Rand());
  auto sink = logging::impl::BufferedFileSink(filename);
  for ([[maybe_unused]] auto _ : state) {
    for (auto i = 0; i < kCountLogs; ++i) {
      sink.Log({"message\n", logging::Level::kWarning});
    }
  }
  sink.Flush();
}
BENCHMARK(check_buffered_file_sink);

USERVER_NAMESPACE_END
