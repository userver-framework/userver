#pragma once

/// @file userver/formats/yaml/inline.hpp
/// @brief Inline value builders
/// @ingroup userver_universal

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

namespace impl {

template <typename T>
concept InlineValue =
    std::is_same_v<T, std::nullptr_t> || std::is_integral_v<T> || std::is_same_v<T, float> ||
    std::is_same_v<T, double> || std::is_convertible_v<T, std::string_view> || std::is_same_v<T, formats::yaml::Value>;

class InlineObjectBuilder {
public:
    InlineObjectBuilder() = default;

    template <typename... Args>
    formats::yaml::Value Build(const Args&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Cannot build an object from an odd number of key-value arguments");
        return DoBuild(args...);
    }

private:
    formats::yaml::Value DoBuild();

    template <typename FieldValue, typename... Tail>
    formats::yaml::Value DoBuild(std::string_view key, const FieldValue& value, Tail&&... tail) {
        Append(key, value);
        return DoBuild(std::forward<Tail>(tail)...);
    }

    template <InlineValue FieldValue>
    void Append(std::string_view key, const FieldValue& value) {
        yaml_[std::string{key}] = value;
    }

    void Append(std::string_view key, const std::chrono::system_clock::time_point&);

    formats::yaml::ValueBuilder yaml_{Type::kObject};
};

class InlineArrayBuilder {
public:
    InlineArrayBuilder() = default;

    formats::yaml::Value Build();

    template <typename... Elements>
    formats::yaml::Value Build(const Elements&... elements) {
        (Append(elements), ...);
        return Build();
    }

private:
    template <InlineValue Element>
    void Append(const Element& value) {
        yaml_.PushBack(value);
    }

    void Append(const std::chrono::system_clock::time_point&);

    formats::yaml::ValueBuilder yaml_{Type::kArray};
};

}  // namespace impl

/// @ingroup userver_formats
///
/// Constructs an object Value from provided key-value pairs
template <typename... Args>
Value MakeObject(Args&&... args) {
    return impl::InlineObjectBuilder().Build(std::forward<Args>(args)...);
}

/// @ingroup userver_formats
///
/// Constructs an array Value from provided element list
template <typename... Args>
Value MakeArray(Args&&... args) {
    return impl::InlineArrayBuilder().Build(std::forward<Args>(args)...);
}

}  // namespace formats::yaml

USERVER_NAMESPACE_END
