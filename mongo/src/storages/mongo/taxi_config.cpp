#include <storages/mongo/taxi_config.hpp>

namespace storages::mongo::impl {

TaxiConfig::TaxiConfig(const taxi_config::DocsMap& docs_map)
    : connect_precheck_enabled(
          docs_map.Get("USERVER_MONGO_CONNECT_PRECHECK_ENABLED").As<bool>()) {}

}  // namespace storages::mongo::impl
