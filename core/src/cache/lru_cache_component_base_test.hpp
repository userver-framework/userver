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

 private:
  Value DoGetByKey(const Key& key) override {
    return GetValueForExpiredKeyFromRemote(key);
  }

  Value GetValueForExpiredKeyFromRemote(const Key& key);
};
/// [Sample lru cache component]

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
inline Value ExampleCacheComponent::GetValueForExpiredKeyFromRemote(
    const Key&) {
  return 0;
}

USERVER_NAMESPACE_BEGIN

template <>
inline constexpr bool components::kHasValidate<ExampleCacheComponent> = true;

USERVER_NAMESPACE_END
