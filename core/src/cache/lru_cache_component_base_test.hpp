#include <userver/utest/using_namespace_userver.hpp>

/// [Sample lru cache component]
#include <userver/cache/lru_cache_component_base.hpp>
#include <userver/components/component_list.hpp>

using Key = std::string;
using Value = std::size_t;

class ExampleCacheComponent final
    : public cache::LruCacheComponent<Key, Value> {
 public:
  static constexpr auto kName = "example-cache";

  ExampleCacheComponent(const components::ComponentConfig& config,
                        const components::ComponentContext& context)
      : ::cache::LruCacheComponent<Key, Value>(config, context) {}

  static std::string GetStaticConfigSchema();

 private:
  Value DoGetByKey(const Key& key) override {
    return GetValueForExpiredKeyFromRemote(key);
  }

  Value GetValueForExpiredKeyFromRemote(const Key& key);
};
/// [Sample lru cache component]

Value ExampleCacheComponent::GetValueForExpiredKeyFromRemote(const Key&) {
  return 0;
}

std::string ExampleCacheComponent::GetStaticConfigSchema() {
  return R"(
type: object
description: example-cache config
additionalProperties: false
properties:
    size:
        type: integer
        description: max amount of items to store in cache
    ways:
        type: integer
        description: number of ways for associative cache
    lifetime:
        type: string
        description: TTL for cache entries (0 is unlimited)
        defaultDescription: 0
    config-settings:
        type: boolean
        description: enables dynamic reconfiguration with CacheConfigSet
        defaultDescription: true
)";
}

USERVER_NAMESPACE_BEGIN

template <>
inline constexpr bool components::kHasValidate<ExampleCacheComponent> = true;

USERVER_NAMESPACE_END
