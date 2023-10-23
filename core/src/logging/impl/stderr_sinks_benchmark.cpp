#include <benchmark/benchmark.h>

#include <userver/fs/blocking/c_file.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/temp_file.hpp>

#include "buffered_file_sink.hpp"
#include "fd_sink.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::size_t kCountLogs = 10;
}  // namespace

void LogFdSink(benchmark::State& state) {
  auto fd_scope = fs::blocking::FileDescriptor::Open(
      "/dev/null", fs::blocking::OpenFlag::kWrite);

  auto sink = logging::impl::UnownedFdSink(fd_scope.GetNative());
  for ([[maybe_unused]] auto _ : state) {
    for (std::size_t i = 0; i < kCountLogs; ++i) {
      sink.Log({"message\n", logging::Level::kWarning});
    }
  }
  sink.Flush();
}
BENCHMARK(LogFdSink);

void LogBufferedFdSink(benchmark::State& state) {
  const auto file_scope = fs::blocking::TempFile::Create();
  fs::blocking::CFile c_file_scope{"/dev/null", fs::blocking::OpenFlag::kWrite};

  auto sink = logging::impl::BufferedUnownedFileSink(c_file_scope.GetNative());
  for ([[maybe_unused]] auto _ : state) {
    for (std::size_t i = 0; i < kCountLogs; ++i) {
      sink.Log({"message\n", logging::Level::kWarning});
    }
  }
  sink.Flush();
}
BENCHMARK(LogBufferedFdSink);

USERVER_NAMESPACE_END
