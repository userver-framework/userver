#pragma once

#include <benchmark/benchmark.h>

#include <taxi_config/config.hpp>

namespace taxi_config {

taxi_config::Config MakeTaxiConfig(const taxi_config::DocsMap& docs_map);

std::shared_ptr<const taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map);

}  // namespace taxi_config
