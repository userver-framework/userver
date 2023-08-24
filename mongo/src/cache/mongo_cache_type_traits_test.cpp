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
      const std::chrono::system_clock::duration& correction);
};

struct IncorrectReturnTypeOfFindOperation {
  static int GetFindOperation(
      cache::UpdateType type,
      const std::chrono::system_clock::time_point& last_update,
      const std::chrono::system_clock::time_point& now,
      const std::chrono::system_clock::duration& correction);
};

struct IncorrectSignatureOfFindOperation {
  static storages::mongo::operations::Find GetFindOperation(int x, int y);
};

TEST(CheckTraits, DeserializeObject) {
  EXPECT_TRUE(mongo_cache::impl::kHasCorrectDeserializeObject<
              CorrectDeserializeObject>);
  EXPECT_FALSE(mongo_cache::impl::kHasCorrectDeserializeObject<
               IncorrectReturnTypeOfDeserializeObject>);
  EXPECT_FALSE(mongo_cache::impl::kHasCorrectDeserializeObject<
               IncorrectSignatureOfFindOperation>);
}

TEST(CheckTraits, FindOperation) {
  EXPECT_TRUE(
      mongo_cache::impl::kHasCorrectFindOperation<CorrectMongoCacheTraits>);
  EXPECT_FALSE(mongo_cache::impl::kHasCorrectFindOperation<
               IncorrectReturnTypeOfFindOperation>);
  EXPECT_FALSE(mongo_cache::impl::kHasCorrectFindOperation<
               IncorrectSignatureOfFindOperation>);
}

TEST(CheckTraits, CorrectTraits) {
  mongo_cache::impl::CheckTraits<CorrectMongoCacheTraits>{};
}

USERVER_NAMESPACE_END
