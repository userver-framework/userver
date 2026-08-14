#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <userver/cache/base_mongo_cache.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace functional_tests {

/// The key of the documents that runtime-query-mongo-cache loads. It is a
/// dynamic config, so the query of the cache is not known at compile time and
/// changes while the service is running.
const dynamic_config::Key kCacheKeyFilter{"TEST_MONGO_CACHE_KEY_FILTER", std::string{"second"}};

struct CachedObject {
    std::string key;
    int value{0};
};

/// @brief The component that owns the mongo collections of a service is
/// generated outside of userver, so the test service defines its own.
class MongoCollections final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "mongo-collections";

    struct Collections {
        storages::mongo::Collection cached_documents;
    };

    MongoCollections(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ComponentBase(config, context),
          collections_(std::make_shared<Collections>(Collections{
              context.FindComponent<components::Mongo>("cache-database").GetPool()->GetCollection("cached_documents"),
          }))
    {}

    template <typename T>
    std::shared_ptr<T> GetCollectionForLibrary() const {
        static_assert(std::is_same_v<T, Collections>);
        return collections_;
    }

private:
    const std::shared_ptr<Collections> collections_;
};

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

// The traits define no query, so the cache cannot be used as is: the compiler
// requires an implementation of MakeFindOperation in a derived component
static_assert(std::is_abstract_v<components::MongoCache<RuntimeQueryCacheTraits>>);

/// @brief A cache that builds its query from the dynamic config, i.e. from a
/// value that is only known at runtime and changes while the service runs.
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

struct DefaultQueryCacheTraits {
    static constexpr std::string_view kName = "default-query-mongo-cache";

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

    static constexpr bool kUseDefaultFindOperation = true;
};

using DefaultQueryCache = components::MongoCache<DefaultQueryCacheTraits>;

// The traits define the query, so no derived component is required
static_assert(!std::is_abstract_v<DefaultQueryCache>);

/// @brief Reports the contents of both caches and how RuntimeQueryCache has
/// been updated, so that the tests can check them.
class CacheStateHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cache-state";

    CacheStateHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          runtime_query_cache_(context.FindComponent<RuntimeQueryCache>()),
          default_query_cache_(context.FindComponent<DefaultQueryCache>())
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);

        formats::json::ValueBuilder response(formats::json::Type::kObject);
        response["runtime-query-cache"] = DumpCache(runtime_query_cache_);
        response["default-query-cache"] = DumpCache(default_query_cache_);
        response["make-find-operation-count"] = runtime_query_cache_.GetMakeFindOperationCount();
        response["last-update-type"] =
            runtime_query_cache_.GetLastUpdateType() == cache::UpdateType::kFull ? "full" : "incremental";
        return formats::json::ToString(response.ExtractValue());
    }

private:
    template <typename Cache>
    static formats::json::Value DumpCache(const Cache& cache) {
        const auto data = cache.Get();
        formats::json::ValueBuilder builder(formats::json::Type::kObject);
        for (const auto& [key, object] : *data) {
            builder[key] = object.value;
        }
        return builder.ExtractValue();
    }

    const RuntimeQueryCache& runtime_query_cache_;
    const DefaultQueryCache& default_query_cache_;
};

}  // namespace functional_tests

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<components::Mongo>("cache-database")
            .Append<functional_tests::MongoCollections>()
            .Append<functional_tests::RuntimeQueryCache>()
            .Append<functional_tests::DefaultQueryCache>()
            .Append<functional_tests::CacheStateHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
