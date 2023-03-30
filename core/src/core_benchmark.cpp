#include <benchmark/benchmark.h>

#include <userver/logging/log.hpp>
#include <userver/utils/impl/static_registration.hpp>

int main(int argc, char** argv) {
  USERVER_NAMESPACE::utils::impl::FinishStaticRegistration();

  USERVER_NAMESPACE::logging::SetDefaultLoggerLevel(
      USERVER_NAMESPACE::logging::Level::kError);

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
