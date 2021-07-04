#include <benchmark/benchmark.h>

#include <userver/logging/log.hpp>

int main(int argc, char** argv) {
  logging::SetDefaultLoggerLevel(logging::Level::kError);

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
