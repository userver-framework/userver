#include <userver/cache/base_mysql_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

std::string GetMySQLCacheSchema() {
  return R"(
type: object
description: MySQL cache component
additionalProperties: false
properties:
    mysql-component:
        type: string
        description: name of the MySQL component to use for the cache
)";
}

}  // namespace components::impl

USERVER_NAMESPACE_END
