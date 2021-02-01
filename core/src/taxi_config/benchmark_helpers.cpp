// Not included, because it includes <benchmark/benchmark.h>, and it's only
// allowed in actual benchmark files
//
// #include <taxi_config/benchmark_helpers.hpp>

#include <taxi_config/config.hpp>

namespace taxi_config {

taxi_config::Config MakeTaxiConfig(const taxi_config::DocsMap& docs_map) {
  return taxi_config::Config(docs_map);
}

std::shared_ptr<const taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map) {
  return std::make_shared<const taxi_config::Config>(MakeTaxiConfig(docs_map));
}

}  // namespace taxi_config
