#pragma once

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/json_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/transform_io.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

template <typename T>
constexpr bool kStoreAsJson = false;

namespace detail {

template <typename T>
struct JsonConverter {
    using PostgresType = formats::json::Value;
    using UserType = T;

    auto operator()(const UserType& user_data) const -> PostgresType {
        return Serialize(user_data, formats::serialize::To<PostgresType>{});
    }

    auto operator()(const PostgresType& db_data) const -> UserType { return db_data.template As<UserType>(); }
};

}  // namespace detail

// A constrained partial specialisation, not an `Enable` parameter: BufferParser
// and BufferFormatter take a single template argument, so every other
// specialisation in this directory matches a type *structurally*. An opt-in is a
// predicate rather than a shape, which is what a requires-clause is for.
template <typename T>
requires(kStoreAsJson<T> && formats::common::impl::HasParse<formats::json::Value, T>)
struct BufferParser<T> : TransformParser<T, formats::json::Value, detail::JsonConverter<T>> {
    using BaseType = TransformParser<T, formats::json::Value, detail::JsonConverter<T>>;
    using BaseType::BaseType;
};

template <typename T>
requires(kStoreAsJson<T> && formats::common::impl::HasSerialize<formats::json::Value, T>)
struct BufferFormatter<T> : TransformFormatter<T, formats::json::Value, detail::JsonConverter<T>> {
    using BaseType = TransformFormatter<T, formats::json::Value, detail::JsonConverter<T>>;
    using BaseType::BaseType;
};

namespace traits {
namespace detail {

template <typename T>
constexpr bool kHasJsonParseMapping = kStoreAsJson<T> && formats::common::impl::HasParse<formats::json::Value, T>;

template <typename T>
constexpr bool
    kHasJsonSerializeMapping = kStoreAsJson<T> && formats::common::impl::HasSerialize<formats::json::Value, T>;

template <typename T>
constexpr bool kIsMappedToJson = kStoreAsJson<T> && kHasJsonParseMapping<T> && kHasJsonSerializeMapping<T>;

}  // namespace detail

template <typename T>
requires detail::kIsMappedToJson<T>
struct IsMappedToPg<T> : std::true_type {};

template <typename T>
requires detail::kIsMappedToJson<T>
struct IsSpecialMapping<T> : std::true_type {};

}  // namespace traits

template <typename T>
requires traits::detail::kIsMappedToJson<T>
struct CppToPg<T> : CppToPg<formats::json::Value> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
