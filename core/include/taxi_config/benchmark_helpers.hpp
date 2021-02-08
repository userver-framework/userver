#pragma once

#include <benchmark/benchmark.h>

#include <taxi_config/config.hpp>
#include <utils/shared_readable_ptr.hpp>

namespace taxi_config {

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map);

}  // namespace taxi_config
