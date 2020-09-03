#pragma once

#include <type_traits>

#include <utils/meta.hpp>
#include <utils/void_t.hpp>

namespace mongo_cache::impl {

template <typename T, typename = ::utils::void_t<>>
struct HasCollectionsField : std::false_type {};
template <typename T>
struct HasCollectionsField<T,
                           ::utils::void_t<decltype(T::kMongoCollectionsField)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasCollectionsField = HasCollectionsField<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasUpdateFieldName : std::false_type {};
template <typename T>
struct HasUpdateFieldName<T,
                          ::utils::void_t<decltype(T::kMongoUpdateFieldName)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasUpdateFieldName = HasUpdateFieldName<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasKeyField : std::false_type {};
template <typename T>
struct HasKeyField<T, ::utils::void_t<decltype(T::kKeyField)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasKeyField = HasKeyField<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasValidDataType : std::false_type {};
template <typename T>
struct HasValidDataType<T, ::utils::void_t<typename T::DataType>>
    : meta::IsMap<typename T::DataType> {};
template <typename T>
constexpr bool kHasValidDataType = HasValidDataType<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasSecondaryPreferred : std::false_type {};
template <typename T>
struct HasSecondaryPreferred<
    T, ::utils::void_t<decltype(T::kIsSecondaryPreferred)>>
    : meta::IsBool<std::decay_t<decltype(T::kIsSecondaryPreferred)>> {};
template <typename T>
constexpr bool kHasSecondaryPreferred = HasSecondaryPreferred<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasDeserializeObject : std::false_type {};
template <typename T>
struct HasDeserializeObject<T, ::utils::void_t<decltype(T::DeserializeObject)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasDeserializeObject = HasDeserializeObject<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasDefaultDeserializeObject : std::false_type {};
template <typename T>
struct HasDefaultDeserializeObject<
    T, ::utils::void_t<decltype(T::kUseDefaultDeserializeObject)>>
    : meta::IsBool<std::decay_t<decltype(T::kUseDefaultDeserializeObject)>> {};
template <typename T>
constexpr bool kHasDefaultDeserializeObject =
    HasDefaultDeserializeObject<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasFindOperation : std::false_type {};
template <typename T>
struct HasFindOperation<T, ::utils::void_t<decltype(T::GetFindOperation)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasFindOperation = HasFindOperation<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasDefaultFindOperation : std::false_type {};
template <typename T>
struct HasDefaultFindOperation<
    T, ::utils::void_t<decltype(T::kUseDefaultFindOperation)>>
    : meta::IsBool<std::decay_t<decltype(T::kUseDefaultFindOperation)>> {};
template <typename T>
constexpr bool kHasDefaultFindOperation = HasDefaultFindOperation<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasInvalidDocumentsSkipped : std::false_type {};
template <typename T>
struct HasInvalidDocumentsSkipped<
    T, ::utils::void_t<decltype(T::kAreInvalidDocumentsSkipped)>>
    : meta::IsBool<std::decay_t<decltype(T::kAreInvalidDocumentsSkipped)>> {};
template <typename T>
constexpr bool kHasInvalidDocumentsSkipped =
    HasInvalidDocumentsSkipped<T>::value;

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
