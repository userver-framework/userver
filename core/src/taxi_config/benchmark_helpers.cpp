// Not included, because it includes <benchmark/benchmark.h>, and it's only
// allowed in actual benchmark files
//
// #include <taxi_config/benchmark_helpers.hpp>

#include <taxi_config/config.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace taxi_config {

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map) {
  return std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(docs_map));
}

}  // namespace taxi_config
