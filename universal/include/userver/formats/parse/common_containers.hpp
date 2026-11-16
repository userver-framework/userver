#pragma once

/// @file userver/formats/parse/common_containers.hpp
/// @brief Parsers and converters for Standard Library containers and
/// std::optional
///
/// @ingroup userver_universal userver_formats_parse

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string_view>
#include <type_traits>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/meta.hpp>

/// @brief Boost.UUID helpers referenced by format parsers and serializers.
namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace utils::impl::strong_typedef {
struct StrongTypedefTag;
}

namespace formats::parse {

namespace impl {

template <typename T, class Value>
inline T AsExtractor(const Value& value) {
    return value.template As<T>();
}

template <typename T, class Value>
inline T ConvertToExtractor(const Value& value) {
    return value.template ConvertTo<T>();
}

template <typename Container, typename Value>
void ReserveForParse(Container& container, const Value& value) {
    if constexpr (requires { container.reserve(value.GetSize()); }) {
        auto size = value.GetSize();
        if constexpr (requires { container.max_size(); }) {
            size = std::min<std::size_t>(size, container.max_size());
        }
        container.reserve(size);
    }
}

template <typename ArrayType, class Value, typename ExtractFunc>
ArrayType ParseArray(const Value& value, ExtractFunc extract_func) {
    value.CheckArrayOrNull();
    ArrayType response;
    const auto end = value.end();
    impl::ReserveForParse(response, value);
    auto inserter = std::inserter(response, response.end());

    for (auto it = value.begin(); it != end; ++it) {
        *inserter = extract_func(*it);
        ++inserter;
    }

    return response;
}

template <typename ObjectType, class Value, typename ExtractFunc>
ObjectType ParseObject(const Value& value, ExtractFunc extract_func) {
    using KeyType = typename ObjectType::key_type;

    value.CheckObjectOrNull();
    ObjectType result;

    const auto end = value.end();
    impl::ReserveForParse(result, value);
    for (auto it = value.begin(); it != end; ++it) {
        if constexpr (std::is_constructible_v<KeyType, std::string>) {
            result.emplace(it.GetName(), extract_func(*it));
        } else {
            result.emplace(Parse(std::string_view(it.GetName()), To<KeyType>{}), extract_func(*it));
        }
    }

    return result;
}

template <typename T>
concept RangeNotMap =
    meta::IsRange<T> && !meta::IsMap<T> && !std::is_same_v<T, boost::uuids::uuid> &&
    !std::is_convertible_v<T&, utils::impl::strong_typedef::StrongTypedefTag&>;

}  // namespace impl

template <impl::RangeNotMap T, common::IsFormatValue Value>
T Parse(const Value& value, To<T>) {
    return impl::ParseArray<T>(value, &impl::AsExtractor<meta::RangeValueType<T>, Value>);
}

template <meta::IsMap T, common::IsFormatValue Value>
T Parse(const Value& value, To<T>) {
    return impl::ParseObject<T>(value, &impl::AsExtractor<typename T::mapped_type, Value>);
}

template <typename T, typename Value>
std::optional<decltype(Parse(std::declval<Value>(), To<T>{}))> Parse(const Value& value, To<std::optional<T>>) {
    if (value.IsMissing() || value.IsNull()) {
        return std::nullopt;
    }
    return value.template As<T>();
}

template <class Value>
std::optional<std::nullptr_t> Parse(const Value&, To<std::optional<std::nullptr_t>>) {
    static_assert(!sizeof(Value), "optional<nullptr_t> is forbidden, check IsNull() instead");
    return nullptr;
}

template <impl::RangeNotMap T, typename Value>
T Convert(const Value& value, To<T>) {
    if (value.IsMissing()) {
        return {};
    }
    return impl::ParseArray<T>(value, &impl::ConvertToExtractor<meta::RangeValueType<T>, Value>);
}

template <meta::IsMap T, typename Value>
T Convert(const Value& value, To<T>) {
    if (value.IsMissing()) {
        return {};
    }
    return impl::ParseObject<T>(value, &impl::ConvertToExtractor<typename T::mapped_type, Value>);
}

template <typename T, typename Value>
std::optional<T> Convert(const Value& value, To<std::optional<T>>) {
    if (value.IsMissing() || value.IsNull()) {
        return std::nullopt;
    }
    return value.template ConvertTo<T>();
}

template <class Value>
std::optional<std::nullptr_t> Convert(const Value&, To<std::optional<std::nullptr_t>>) {
    static_assert(!sizeof(Value), "optional<nullptr_t> is forbidden, check IsNull() instead");
    return nullptr;
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
