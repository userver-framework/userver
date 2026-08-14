#include <userver/cache/mongo_cache_type_traits.hpp>

#include <userver/cache/update_type.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/storages/mongo/operations.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

struct CorrectDeserializeObject {
    using ObjectType = int;

    static ObjectType DeserializeObject(const formats::bson::Document&);
};

struct IncorrectReturnTypeOfDeserializeObject {
    static void DeserializeObject(const formats::bson::Document&);
};

struct IncorrectSignatureOfDeserializeObject {
    using ObjectType = int;

    static ObjectType DeserializeObject(int x, const formats::bson::Document&);
};

// Do not use it as example of MongoCacheTraits
struct CorrectMongoCacheTraits {
    static constexpr int kMongoCollectionsField = 0;
    static constexpr int kKeyField = 0;
    using DataType = std::unordered_map<int, int>;

    static constexpr bool kIsSecondaryPreferred = true;
    static constexpr bool kAreInvalidDocumentsSkipped = true;
    static constexpr bool kUseDefaultDeserializeObject = true;

    static storages::mongo::operations::Find GetFindOperation(
        cache::UpdateType type,
        const std::chrono::system_clock::time_point& last_update,
        const std::chrono::system_clock::time_point& now,
        const std::chrono::system_clock::duration& correction
    );
};

// Neither GetFindOperation nor kUseDefaultFindOperation: the query is built by
// an override of MongoCache::MakeFindOperation in a derived component
struct MongoCacheTraitsWithoutFindOperation {
    static constexpr int kMongoCollectionsField = 0;
    static constexpr int kKeyField = 0;
    using DataType = std::unordered_map<int, int>;

    static constexpr bool kIsSecondaryPreferred = true;
    static constexpr bool kAreInvalidDocumentsSkipped = true;
    static constexpr bool kUseDefaultDeserializeObject = true;
};

struct IncorrectReturnTypeOfFindOperation {
    static int GetFindOperation(
        cache::UpdateType type,
        const std::chrono::system_clock::time_point& last_update,
        const std::chrono::system_clock::time_point& now,
        const std::chrono::system_clock::duration& correction
    );
};

struct IncorrectSignatureOfFindOperation {
    static storages::mongo::operations::Find GetFindOperation(int x, int y);
};

TEST(CheckTraits, DeserializeObject) {
    EXPECT_TRUE(mongo_cache::impl::HasCorrectDeserializeObject<CorrectDeserializeObject>);
    EXPECT_FALSE(mongo_cache::impl::HasCorrectDeserializeObject<IncorrectReturnTypeOfDeserializeObject>);
    EXPECT_FALSE(mongo_cache::impl::HasCorrectDeserializeObject<IncorrectSignatureOfFindOperation>);
}

TEST(CheckTraits, FindOperation) {
    EXPECT_TRUE(mongo_cache::impl::HasCorrectFindOperation<CorrectMongoCacheTraits>);
    EXPECT_FALSE(mongo_cache::impl::HasCorrectFindOperation<IncorrectReturnTypeOfFindOperation>);
    EXPECT_FALSE(mongo_cache::impl::HasCorrectFindOperation<IncorrectSignatureOfFindOperation>);
}

TEST(CheckTraits, FindOperationInTraits) {
    EXPECT_TRUE(mongo_cache::impl::HasFindOperationInTraits<CorrectMongoCacheTraits>);
    EXPECT_FALSE(mongo_cache::impl::HasFindOperationInTraits<MongoCacheTraitsWithoutFindOperation>);
}

TEST(CheckTraits, CorrectTraits) { mongo_cache::impl::CheckTraits<CorrectMongoCacheTraits>{}; }

// The find operation is optional in the traits: MongoCache::MakeFindOperation
// is then pure virtual, and the compiler requires an override in a derived
// component. See mongo/functional_tests/cache for the tests of such a cache
TEST(CheckTraits, TraitsWithoutFindOperation) {
    mongo_cache::impl::CheckTraits<MongoCacheTraitsWithoutFindOperation>{};
}

USERVER_NAMESPACE_END
