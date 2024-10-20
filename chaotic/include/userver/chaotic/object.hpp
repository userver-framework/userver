#pragma once

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename BuilderFunc, typename Value>
Value ExtractAdditionalPropertiesTrue(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    typename Value::Builder builder(formats::common::Type::kObject);

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) continue;

        builder[name] = value;
    }
    return builder.ExtractValue();
}

template <typename BuilderFunc, typename Value>
void ValidateNoAdditionalProperties(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) continue;

        throw std::runtime_error(fmt::format("Unknown property '{}'", name));
    }
}

template <typename T, template <typename...> typename Map, typename Value, typename BuilderFunc>
Map<std::string, formats::common::ParseType<Value, T>>
ExtractAdditionalProperties(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    Map<std::string, formats::common::ParseType<Value, T>> map;

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) continue;

        map.emplace(name, value.template As<T>());
    }
    return map;
}

}  // namespace chaotic

USERVER_NAMESPACE_END
