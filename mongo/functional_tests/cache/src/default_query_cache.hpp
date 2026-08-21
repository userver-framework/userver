#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <userver/cache/base_mongo_cache.hpp>
#include <userver/formats/bson/document.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <cached_object.hpp>
#include <mongo_collections.hpp>

namespace functional_tests {

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

}  // namespace functional_tests
