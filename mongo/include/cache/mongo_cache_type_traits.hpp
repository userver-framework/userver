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
    : meta::is_map<typename T::DataType> {};
template <typename T>
constexpr bool kHasValidDataType = HasValidDataType<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasDeserializeFunc : std::false_type {};
template <typename T>
struct HasDeserializeFunc<T, ::utils::void_t<decltype(T::kDeserializeFunc)>>
    : std::true_type {};
template <typename T>
constexpr bool kHasDeserializeFunc = HasDeserializeFunc<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasSecondaryPreferred : std::false_type {};
template <typename T>
struct HasSecondaryPreferred<
    T, ::utils::void_t<std::decay_t<decltype(T::kIsSecondaryPreferred)>>>
    : meta::is_bool<std::decay_t<decltype(T::kIsSecondaryPreferred)>> {};
template <typename T>
constexpr bool kHasSecondaryPreferred = HasSecondaryPreferred<T>::value;

template <typename T, typename = ::utils::void_t<>>
struct HasInvalidDocumentsSkipped : std::false_type {};
template <typename T>
struct HasInvalidDocumentsSkipped<
    T, ::utils::void_t<std::decay_t<decltype(T::kAreInvalidDocumentsSkipped)>>>
    : meta::is_bool<std::decay_t<decltype(T::kAreInvalidDocumentsSkipped)>> {};
template <typename T>
constexpr bool kHasInvalidDocumentsSkipped =
    HasInvalidDocumentsSkipped<T>::value;

template <typename MongoCacheTraits>
struct CheckTraits {
  static_assert(kHasCollectionsField<MongoCacheTraits>,
                "Mongo cache traits must specify collections field");
  static_assert(kHasKeyField<MongoCacheTraits>,
                "Mongo cache traits must specify key field");
  static_assert(kHasValidDataType<MongoCacheTraits>,
                "Mongo cache traits must specify mapping data type");
  static_assert(kHasDeserializeFunc<MongoCacheTraits>,
                "Mongo cache traits must provide deserialization function");
  static_assert(kHasSecondaryPreferred<MongoCacheTraits>,
                "Mongo cache traits must specify read preference");
  static_assert(kHasInvalidDocumentsSkipped<MongoCacheTraits>,
                "Mongo cache traits must specify validation policy");
};

}  // namespace mongo_cache::impl
