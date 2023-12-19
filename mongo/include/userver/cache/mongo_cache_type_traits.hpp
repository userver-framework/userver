#pragma once

#include <chrono>
#include <type_traits>

#include <userver/cache/update_type.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
class Document;
}

namespace storages::mongo::operations {
class Find;
}

namespace mongo_cache::impl {

template <typename T>
using CollectionsField = decltype(T::kMongoCollectionsField);
template <typename T>
inline constexpr bool kHasCollectionsField =
    meta::kIsDetected<CollectionsField, T>;

template <typename T>
using UpdateFieldName = decltype(T::kMongoUpdateFieldName);
template <typename T>
inline constexpr bool kHasUpdateFieldName =
    meta::kIsDetected<UpdateFieldName, T>;

template <typename T>
using KeyField = decltype(T::kKeyField);
template <typename T>
inline constexpr bool kHasKeyField = meta::kIsDetected<KeyField, T>;

template <typename T>
using DataType = typename T::DataType;
template <typename T>
inline constexpr bool kHasValidDataType =
    meta::kIsMap<meta::DetectedType<DataType, T>>;

template <typename T>
using HasSecondaryPreferred = decltype(T::kIsSecondaryPreferred);
template <typename T>
inline constexpr bool kHasSecondaryPreferred =
    meta::kIsDetected<HasSecondaryPreferred, T>;

template <typename T>
using HasDeserializeObject = decltype(T::DeserializeObject);
template <typename T>
inline constexpr bool kHasDeserializeObject =
    meta::kIsDetected<HasDeserializeObject, T>;

template <typename T>
using HasCorrectDeserializeObject =
    meta::ExpectSame<typename T::ObjectType,
                     decltype(std::declval<const T&>().DeserializeObject(
                         std::declval<const formats::bson::Document&>()))>;
template <typename T>
inline constexpr bool kHasCorrectDeserializeObject =
    meta::kIsDetected<HasCorrectDeserializeObject, T>;

template <typename T>
using HasDefaultDeserializeObject = decltype(T::kUseDefaultDeserializeObject);
template <typename T>
inline constexpr bool kHasDefaultDeserializeObject =
    meta::kIsDetected<HasDefaultDeserializeObject, T>;

template <typename T>
using HasFindOperation = decltype(T::GetFindOperation);
template <typename T>
inline constexpr bool kHasFindOperation =
    meta::kIsDetected<HasFindOperation, T>;

template <typename T>
using HasCorrectFindOperation = meta::ExpectSame<
    storages::mongo::operations::Find,
    decltype(std::declval<const T&>().GetFindOperation(
        std::declval<cache::UpdateType>(),
        std::declval<const std::chrono::system_clock::time_point&>(),
        std::declval<const std::chrono::system_clock::time_point&>(),
        std::declval<const std::chrono::system_clock::duration&>()))>;

template <typename T>
inline constexpr bool kHasCorrectFindOperation =
    meta::kIsDetected<HasCorrectFindOperation, T>;

template <typename T>
using HasDefaultFindOperation = decltype(T::kUseDefaultFindOperation);
template <typename T>
inline constexpr bool kHasDefaultFindOperation =
    meta::kIsDetected<HasDefaultFindOperation, T>;

template <typename T>
using HasInvalidDocumentsSkipped = decltype(T::kAreInvalidDocumentsSkipped);
template <typename T>
inline constexpr bool kHasInvalidDocumentsSkipped =
    meta::kIsDetected<HasInvalidDocumentsSkipped, T>;

template <typename>
struct ClassByMemberPointer {};
template <typename T, typename C>
struct ClassByMemberPointer<T C::*> {
  using type = C;
};
template <typename CollectionPtr>
using CollectionsType =
    typename ClassByMemberPointer<std::remove_cv_t<CollectionPtr>>::type;

template <typename MongoCacheTraits>
struct CheckTraits {
  CheckTraits() {
    if constexpr (kHasDefaultDeserializeObject<MongoCacheTraits>) {
      static_assert(
          std::is_same_v<
              std::decay_t<
                  decltype(MongoCacheTraits::kUseDefaultDeserializeObject)>,
              bool>,
          "Mongo cache traits must specify kUseDefaultDeserializeObject as "
          "bool");
    }
    if constexpr (kHasDefaultFindOperation<MongoCacheTraits>) {
      static_assert(
          std::is_same_v<
              std::decay_t<
                  decltype(MongoCacheTraits::kUseDefaultFindOperation)>,
              bool>,
          "Mongo cache traits must specify kUseDefaultFindOperation as bool");
    }
  }

  static_assert(kHasCollectionsField<MongoCacheTraits>,
                "Mongo cache traits must specify collections field");
  static_assert(kHasKeyField<MongoCacheTraits>,
                "Mongo cache traits must specify key field");
  static_assert(kHasValidDataType<MongoCacheTraits>,
                "Mongo cache traits must specify mapping data type");

  static_assert(kHasSecondaryPreferred<MongoCacheTraits>,
                "Mongo cache traits must specify read preference");
  static_assert(
      std::is_same_v<
          std::decay_t<decltype(MongoCacheTraits::kIsSecondaryPreferred)>,
          bool>,
      "Mongo cache traits must specify read preference of a bool type");

  static_assert(kHasInvalidDocumentsSkipped<MongoCacheTraits>,
                "Mongo cache traits must specify validation policy");
  static_assert(
      std::is_same_v<
          std::decay_t<decltype(MongoCacheTraits::kAreInvalidDocumentsSkipped)>,
          bool>,
      "Mongo cache traits must specify validation policy of a bool type");

  static_assert(kHasFindOperation<MongoCacheTraits> ||
                    kHasDefaultFindOperation<MongoCacheTraits>,
                "Mongo cache traits must specify find operation");
  static_assert(!kHasFindOperation<MongoCacheTraits> ||
                    kHasCorrectFindOperation<MongoCacheTraits>,
                "Mongo cache traits must specify find operation with correct "
                "signature and return value type: "
                "static storages::mongo::operations::Find GetFindOperation("
                "cache::UpdateType type, "
                "const std::chrono::system_clock::time_point& last_update, "
                "const std::chrono::system_clock::time_point& now, "
                "const std::chrono::system_clock::duration& correction)");

  static_assert(kHasDeserializeObject<MongoCacheTraits> ||
                    kHasDefaultDeserializeObject<MongoCacheTraits>,
                "Mongo cache traits must specify deserialize object");
  static_assert(
      !kHasDeserializeObject<MongoCacheTraits> ||
          kHasCorrectDeserializeObject<MongoCacheTraits>,
      "Mongo cache traits must specify deserialize object with correct "
      "signature and return value type: "
      "static ObjectType DeserializeObject(const formats::bson::Document& "
      "doc)");
};

}  // namespace mongo_cache::impl

USERVER_NAMESPACE_END
