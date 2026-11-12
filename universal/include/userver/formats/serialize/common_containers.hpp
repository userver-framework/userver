#pragma once

/// @file userver/formats/serialize/common_containers.hpp
/// @brief Serializers for standard containers and optional
/// @ingroup userver_universal userver_formats_serialize

#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#include <userver/compiler/impl/nodebug.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/meta.hpp>

namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace utils::impl::strong_typedef {
struct StrongTypedefTag;
}

/// Common serializers
namespace formats::serialize {

namespace impl {

template <typename T>
concept RangeNotMap =
    meta::IsRange<T> && !meta::IsMap<T> && !std::is_same_v<T, boost::uuids::uuid> &&
    !std::is_convertible_v<T&, utils::impl::strong_typedef::StrongTypedefTag&>;

}

/// Common containers serialization (vector/set)
template <impl::RangeNotMap T, typename Value>
Value Serialize(const T& value, To<Value>) {
    typename Value::Builder builder(formats::common::Type::kArray);
    if constexpr (requires { builder.Reserve(std::ranges::size(value)); }) {
        builder.Reserve(std::ranges::size(value));
    }
    for (const auto& item : value) {
        // explicit cast for vector<bool> shenanigans
        builder.PushBack(static_cast<const meta::RangeValueType<T>&>(item));
    }
    return builder.ExtractValue();
}

/// Mappings serialization
template <meta::IsUniqueMap T, typename Value>
Value Serialize(const T& value, To<Value>) {
    using Key = typename T::key_type;
    typename Value::Builder builder(formats::common::Type::kObject);
    if constexpr (requires { builder.Reserve(std::ranges::size(value)); }) {
        builder.Reserve(std::ranges::size(value));
    }
    for (const auto& [key, value] : value) {
        if constexpr ((std::is_same_v<Key, std::string> || std::is_same_v<Key, std::string_view>) &&
                      requires { builder.EmplaceNocheck(key, value); })
        {
            builder.EmplaceNocheck(key, value);
        } else {
            builder[key] = value;
        }
    }
    return builder.ExtractValue();
}

/// std::optional serialization
template <typename T, typename Value>
USERVER_IMPL_NODEBUG_INLINE_FUNC inline Value Serialize(const std::optional<T>& value, To<Value>) {
    if (!value) {
        return {};
    }

    return typename Value::Builder(*value).ExtractValue();
}

}  // namespace formats::serialize

USERVER_NAMESPACE_END
