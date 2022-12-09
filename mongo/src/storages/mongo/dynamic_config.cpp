#include "dynamic_config.hpp"

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

std::chrono::milliseconds ParseDefaultMaxTime(
    const dynamic_config::DocsMap& docs_map) {
  return std::chrono::milliseconds{
      docs_map.Get("MONGO_DEFAULT_MAX_TIME_MS").As<std::uint32_t>()};
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
