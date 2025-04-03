#include "pool_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

PoolConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<PoolConfig>) {
    PoolConfig config;
    config.initial_size = value["initial_size"].As<size_t>(config.initial_size);
    config.max_size = value["max_size"].As<size_t>(config.max_size);
    config.stack_size = value["stack_size"].As<size_t>(config.stack_size);
    config.local_cache_size = value["local_cache_size"].As<size_t>(config.local_cache_size);
    config.is_stack_usage_monitor_enabled =
        value["stack_usage_monitor_enabled"].As<bool>(config.is_stack_usage_monitor_enabled);
    return config;
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
