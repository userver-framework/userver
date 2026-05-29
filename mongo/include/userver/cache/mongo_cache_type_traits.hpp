#pragma once

/// @file userver/cache/mongo_cache_type_traits.hpp
/// @brief Compile-time concepts and static checks for BaseMongoCache traits

#include <chrono>
#include <type_traits>

#include <userver/cache/update_type.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {
class Document;
}

namespace storages::mongo::operations {
class Find;
}

namespace mongo_cache::impl {

template <typename T>
concept HasCollectionsField = requires { T::kMongoCollectionsField; };

template <typename T>
concept HasUpdateFieldName = requires { T::kMongoUpdateFieldName; };

template <typename T>
concept HasKeyField = requires { T::kKeyField; };

template <typename T>
concept HasValidDataType = meta::kIsMap<typename T::DataType>;

template <typename T>
concept HasSecondaryPreferred = requires { T::kIsSecondaryPreferred; };

template <typename T>
concept HasDeserializeObject = requires { T::DeserializeObject; };

template <typename T>
concept HasCorrectDeserializeObject = requires(const T& t, const formats::bson::Document& doc) {
    {
        t.DeserializeObject(doc)
    } -> std::same_as<typename T::ObjectType>;
};

template <typename T>
concept HasDefaultDeserializeObject = requires { T::kUseDefaultDeserializeObject; };

template <typename T>
concept HasFindOperation = requires { T::GetFindOperation; };

template <typename T>
concept HasCorrectFindOperation = requires(
    const T& t,
    cache::UpdateType update_type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now,
    const std::chrono::system_clock::duration& correction
) {
    // NOLINTNEXTLINE(readability-static-accessed-through-instance)
    {
        t.GetFindOperation(update_type, last_update, now, correction)
    } -> std::same_as<storages::mongo::operations::Find>;
};

template <typename T>
concept HasDefaultFindOperation = requires { T::kUseDefaultFindOperation; };

template <typename T>
concept HasInvalidDocumentsSkipped = requires { T::kAreInvalidDocumentsSkipped; };

template <typename>
struct ClassByMemberPointer {};
template <typename T, typename C>
struct ClassByMemberPointer<T C::*> {
    using type = C;
};
template <typename CollectionPtr>
using CollectionsType = typename ClassByMemberPointer<std::remove_cv_t<CollectionPtr>>::type;

template <typename MongoCacheTraits>
struct CheckTraits {
    CheckTraits() {
        if constexpr (HasDefaultDeserializeObject<MongoCacheTraits>) {
            static_assert(
                std::is_same_v<std::decay_t<decltype(MongoCacheTraits::kUseDefaultDeserializeObject)>, bool>,
                "Mongo cache traits must specify kUseDefaultDeserializeObject as "
                "bool"
            );
        }
        if constexpr (HasDefaultFindOperation<MongoCacheTraits>) {
            static_assert(
                std::is_same_v<std::decay_t<decltype(MongoCacheTraits::kUseDefaultFindOperation)>, bool>,
                "Mongo cache traits must specify kUseDefaultFindOperation as bool"
            );
        }
    }

    static_assert(HasCollectionsField<MongoCacheTraits>, "Mongo cache traits must specify collections field");
    static_assert(HasKeyField<MongoCacheTraits>, "Mongo cache traits must specify key field");
    static_assert(HasValidDataType<MongoCacheTraits>, "Mongo cache traits must specify mapping data type");

    static_assert(HasSecondaryPreferred<MongoCacheTraits>, "Mongo cache traits must specify read preference");
    static_assert(
        std::is_same_v<std::decay_t<decltype(MongoCacheTraits::kIsSecondaryPreferred)>, bool>,
        "Mongo cache traits must specify read preference of a bool type"
    );

    static_assert(HasInvalidDocumentsSkipped<MongoCacheTraits>, "Mongo cache traits must specify validation policy");
    static_assert(
        std::is_same_v<std::decay_t<decltype(MongoCacheTraits::kAreInvalidDocumentsSkipped)>, bool>,
        "Mongo cache traits must specify validation policy of a bool type"
    );

    static_assert(
        HasFindOperation<MongoCacheTraits> || HasDefaultFindOperation<MongoCacheTraits>,
        "Mongo cache traits must specify find operation"
    );
    static_assert(
        !HasFindOperation<MongoCacheTraits> || HasCorrectFindOperation<MongoCacheTraits>,
        "Mongo cache traits must specify find operation with correct "
        "signature and return value type: "
        "static storages::mongo::operations::Find GetFindOperation("
        "cache::UpdateType type, "
        "const std::chrono::system_clock::time_point& last_update, "
        "const std::chrono::system_clock::time_point& now, "
        "const std::chrono::system_clock::duration& correction)"
    );

    static_assert(
        HasDeserializeObject<MongoCacheTraits> || HasDefaultDeserializeObject<MongoCacheTraits>,
        "Mongo cache traits must specify deserialize object"
    );
    static_assert(
        !HasDeserializeObject<MongoCacheTraits> || HasCorrectDeserializeObject<MongoCacheTraits>,
        "Mongo cache traits must specify deserialize object with correct "
        "signature and return value type: "
        "static ObjectType DeserializeObject(const formats::bson::Document& "
        "doc)"
    );
};

}  // namespace mongo_cache::impl

USERVER_NAMESPACE_END
