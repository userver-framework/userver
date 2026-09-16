#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <userver/cache/base_mongo_cache.hpp>
#include <userver/components/component.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/operations.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <cached_object.hpp>
#include <mongo_collections.hpp>

namespace functional_tests {

/// The key of the documents that runtime-query-mongo-cache loads. It is a
/// dynamic config, so the query of the cache is not known at compile time and
/// changes while the service is running.
const dynamic_config::Key kCacheKeyFilter{"TEST_MONGO_CACHE_KEY_FILTER", std::string{"second"}};

/// [RuntimeQueryCache traits]
struct RuntimeQueryCacheTraits {
    static constexpr std::string_view kName = "runtime-query-mongo-cache";

    static constexpr auto kMongoCollectionsField = &MongoCollections::Collections::cached_documents;
    using MongoCollectionsComponent = MongoCollections;

    using ObjectType = CachedObject;
    static constexpr auto kKeyField = &CachedObject::key;
    using KeyType = std::string;
    using DataType = std::unordered_map<KeyType, ObjectType>;

    static constexpr bool kIsSecondaryPreferred = false;
    static constexpr bool kAreInvalidDocumentsSkipped = false;

    static ObjectType DeserializeObject(const formats::bson::Document& doc) {
        return CachedObject{doc["key"].As<std::string>(), doc["value"].As<int>()};
    }

    // Neither GetFindOperation nor kUseDefaultFindOperation: the query is only
    // known at runtime, see RuntimeQueryCache::MakeFindOperation
};
/// [RuntimeQueryCache traits]

// The traits define no query, so the cache cannot be used as is: the compiler
// requires an implementation of MakeFindOperation in a derived component
static_assert(std::is_abstract_v<components::MongoCache<RuntimeQueryCacheTraits>>);

/// @brief A cache that builds its query from the dynamic config, i.e. from a
/// value that is only known at runtime and changes while the service runs.
/// [RuntimeQueryCache]
class RuntimeQueryCache final : public components::MongoCache<RuntimeQueryCacheTraits> {
public:
    RuntimeQueryCache(const components::ComponentConfig& config, const components::ComponentContext& context)
        : MongoCache(config, context),
          config_source_(context.FindComponent<components::DynamicConfig>().GetSource())
    {}

    std::size_t GetMakeFindOperationCount() const { return make_find_operation_count_.load(); }

    cache::UpdateType GetLastUpdateType() const { return last_update_type_.load(); }

protected:
    storages::mongo::operations::Find MakeFindOperation(
        cache::UpdateType type,
        const std::chrono::system_clock::time_point& /*last_update*/,
        const std::chrono::system_clock::time_point& /*now*/,
        const std::chrono::system_clock::duration& /*correction*/
    ) override {
        ++make_find_operation_count_;
        last_update_type_ = type;

        const auto config = config_source_.GetSnapshot();
        return storages::mongo::operations::Find(formats::bson::MakeDoc("key", config[kCacheKeyFilter]));
    }

private:
    const dynamic_config::Source config_source_;
    std::atomic<std::size_t> make_find_operation_count_{0};
    std::atomic<cache::UpdateType> last_update_type_{cache::UpdateType::kFull};
};
/// [RuntimeQueryCache]

}  // namespace functional_tests
