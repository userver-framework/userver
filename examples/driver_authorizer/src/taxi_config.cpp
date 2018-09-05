#include "taxi_config.hpp"

namespace driver_authorizer {

TaxiConfig::TaxiConfig(const DocsMap& docs_map)
    : driver_session_expire_seconds("DRIVER_SESSION_EXPIRE_SECONDS", docs_map),
      driver_session_cc("REDIS_CHECK_SESSION_CONTROL_COMMAND_2", docs_map) {}

}  // namespace driver_authorizer
