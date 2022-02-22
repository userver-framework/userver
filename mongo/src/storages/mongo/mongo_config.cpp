#include "mongo_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

Config::Config(const dynamic_config::DocsMap& docs_map)
    : default_max_time_ms(
          docs_map.Get("MONGO_DEFAULT_MAX_TIME_MS").As<uint32_t>()) {}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
