#pragma once

#include <string>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/trivial_map.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace chaotic {

template <typename BuilderFunc>
formats::json::Value ExtractAdditionalPropertiesTrue(
    const formats::json::Value& json,
    const utils::TrivialSet<BuilderFunc>& names_to_exclude
) {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        builder[name] = value;
    }
    return builder.ExtractValue();
}

template <typename BuilderFunc, typename Value>
formats::json::Value ExtractAdditionalPropertiesTrue(
    const Value& non_json,
    const utils::TrivialSet<BuilderFunc>& names_to_exclude
) {
    return chaotic::ExtractAdditionalPropertiesTrue(non_json.template As<formats::json::Value>(), names_to_exclude);
}

template <typename BuilderFunc, typename Value>
void ValidateNoAdditionalProperties(const Value& json, const utils::TrivialSet<BuilderFunc>& names_to_exclude) {
    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        chaotic::ThrowForValue<Value>(fmt::format("Unknown property '{}'", name), value);
    }
}

template <typename T, template <typename...> typename Map, typename BuilderFunc>
Map<std::string, formats::common::ParseType<formats::json::Value, T>> ExtractAdditionalProperties(
    const formats::json::Value& json,
    const utils::TrivialSet<BuilderFunc>& names_to_exclude
) {
    Map<std::string, formats::common::ParseType<formats::json::Value, T>> map;

    for (const auto& [name, value] : formats::common::Items(json)) {
        if (names_to_exclude.Contains(name)) {
            continue;
        }

        map.emplace(name, value.template As<T>());
    }
    return map;
}

template <typename T, template <typename...> typename Map, typename Value, typename BuilderFunc>
Map<std::string, formats::common::ParseType<formats::json::Value, T>> ExtractAdditionalProperties(
    const Value& non_json,
    const utils::TrivialSet<BuilderFunc>& names_to_exclude
) {
    return chaotic::ExtractAdditionalProperties<T, Map>(non_json.template As<formats::json::Value>(), names_to_exclude);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
