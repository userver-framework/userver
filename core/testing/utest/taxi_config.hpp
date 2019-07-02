#pragma once

#include <taxi_config/config.hpp>

namespace utest {
namespace impl {

// Internal function, don't use it directly, use DefaultTaxiConfig() instead
const taxi_config::Config& ReadDefaultTaxiConfig(const std::string& filename);

}  // namespace impl

/// Get taxi_config::Config with default values
#ifdef DEFAULT_TAXI_CONFIG_FILENAME
inline const taxi_config::Config& GetDefaultTaxiConfig() {
  return impl::ReadDefaultTaxiConfig(DEFAULT_TAXI_CONFIG_FILENAME);
}
#endif

}  // namespace utest
