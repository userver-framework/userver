#include "config.hpp"

namespace taxi_config {

TaxiConfig::TaxiConfig(const DocsMap& docs_map)
    : driver_session_expire_seconds("DRIVER_SESSION_EXPIRE_SECONDS", docs_map) {
}

}  // namespace taxi_config
