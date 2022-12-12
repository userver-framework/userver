#pragma once

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::settings {

struct PoolSettings final {
  std::size_t min_pool_size{1};
  std::size_t max_pool_size{10};
};

}  // namespace storages::mysql::settings

USERVER_NAMESPACE_END
