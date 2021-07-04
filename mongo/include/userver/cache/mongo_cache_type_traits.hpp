#pragma once

#include <type_traits>

#include <userver/utils/meta.hpp>
#include <userver/utils/void_t.hpp>

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
using HasSecondaryPreferred =
    meta::ExpectSame<std::decay_t<decltype(T::kIsSecondaryPreferred)>, bool>;
template <typename T>
inline constexpr bool kHasSecondaryPreferred =
    meta::kIsDetected<HasSecondaryPreferred, T>;

template <typename T>
using HasDeserializeObject = decltype(T::DeserializeObject);
template <typename T>
inline constexpr bool kHasDeserializeObject =
    meta::kIsDetected<HasDeserializeObject, T>;

template <typename T>
using HasDefaultDeserializeObject =
    meta::ExpectSame<std::decay_t<decltype(T::kUseDefaultDeserializeObject)>,
                     bool>;
template <typename T>
inline constexpr bool kHasDefaultDeserializeObject =
    meta::kIsDetected<HasDefaultDeserializeObject, T>;

template <typename T>
using HasFindOperation = decltype(T::GetFindOperation);
template <typename T>
inline constexpr bool kHasFindOperation =
    meta::kIsDetected<HasFindOperation, T>;

template <typename T>
using HasDefaultFindOperation =
    meta::ExpectSame<std::decay_t<decltype(T::kUseDefaultFindOperation)>, bool>;
template <typename T>
inline constexpr bool kHasDefaultFindOperation =
    meta::kIsDetected<HasDefaultFindOperation, T>;

template <typename T>
using HasInvalidDocumentsSkipped =
    meta::ExpectSame<std::decay_t<decltype(T::kAreInvalidDocumentsSkipped)>,
                     bool>;
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
  static_assert(kHasCollectionsField<MongoCacheTraits>,
                "Mongo cache traits must specify collections field");
  static_assert(kHasKeyField<MongoCacheTraits>,
                "Mongo cache traits must specify key field");
  static_assert(kHasValidDataType<MongoCacheTraits>,
                "Mongo cache traits must specify mapping data type");
  static_assert(kHasSecondaryPreferred<MongoCacheTraits>,
                "Mongo cache traits must specify read preference");
  static_assert(kHasInvalidDocumentsSkipped<MongoCacheTraits>,
                "Mongo cache traits must specify validation policy");
  static_assert(kHasFindOperation<MongoCacheTraits> ||
                    kHasDefaultFindOperation<MongoCacheTraits>,
                "Mongo cache traits must specify find operation");
  static_assert(kHasDeserializeObject<MongoCacheTraits> ||
                    kHasDefaultDeserializeObject<MongoCacheTraits>,
                "Mongo cache traits must specify deserialize object");
};

}  // namespace mongo_cache::impl
