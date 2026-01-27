#include <benchmark/benchmark.h>

#include <userver/logging/log.hpp>
#include <userver/utils/impl/static_registration.hpp>

namespace benchmark {

// MaybeReenterWithoutASLR appeared in Google benchmark 1.9.3. Do nothing for older versions.
template <typename... Args>
static void MaybeReenterWithoutASLR(Args&&...) {}

}  // namespace benchmark

int main(int argc, char** argv) {
    ::benchmark::MaybeReenterWithoutASLR(argc, argv);

    USERVER_NAMESPACE::utils::impl::FinishStaticRegistration();

    const USERVER_NAMESPACE::logging::DefaultLoggerLevelScope level_scope{USERVER_NAMESPACE::logging::Level::kError};

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
}
