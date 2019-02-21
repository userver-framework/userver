#include <storages/mongo_ng/pool_config.hpp>

#include <mongoc/mongoc.h>

#include <storages/mongo_ng/exception.hpp>
#include <utils/text.hpp>

namespace storages::mongo_ng {
namespace {

bool IsValidAppName(const std::string& app_name) {
  return app_name.size() <= MONGOC_HANDSHAKE_APPNAME_MAX &&
         utils::text::IsCString(app_name) && utils::text::IsUtf8(app_name);
}

}  // namespace

PoolConfig::PoolConfig(std::string app_name_)
    : conn_timeout_ms(5000),
      so_timeout_ms(5000),
      initial_size(1),
      idle_limit(4),
      app_name(std::move(app_name_)) {
  if (!IsValidAppName(app_name)) {
    throw InvalidConfigException("test") << "Invalid appname in pool config";
  }
}

}  // namespace storages::mongo_ng
