#include <benchmark/benchmark.h>

#include <userver/logging/log.hpp>
#include <userver/utils/impl/static_registration.hpp>

int main(int argc, char** argv) {
  USERVER_NAMESPACE::utils::impl::FinishStaticRegistration();

  const USERVER_NAMESPACE::logging::DefaultLoggerLevelScope level_scope{
      USERVER_NAMESPACE::logging::Level::kError};

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  ::benchmark::RunSpecifiedBenchmarks();
}
