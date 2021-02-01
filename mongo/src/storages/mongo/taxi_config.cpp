#include <storages/mongo/taxi_config.hpp>

#include <exception>

namespace storages::mongo::impl {

TaxiConfig::TaxiConfig(const taxi_config::DocsMap& docs_map) {
  try {
    connect_precheck_enabled =
        docs_map.Get("USERVER_MONGO_CONNECT_PRECHECK_ENABLED").As<bool>();
  } catch (const std::exception&) {
    connect_precheck_enabled = false;
  }
}

}  // namespace storages::mongo::impl
