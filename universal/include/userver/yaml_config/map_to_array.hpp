#pragma once

/// @file userver/yaml_config/map_to_array.hpp
/// @brief @copybrief yaml_config::ParseMapToArray
/// @ingroup userver_universal

#include <utility>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

/// @brief Parses YAML object maps into vectors of named elements.
template <typename T, typename Value>
std::vector<T> ParseMapToArray(const Value& value) {
    value.CheckObjectOrNull();
    std::vector<T> parsed_array;
    parsed_array.reserve(value.GetSize());

    for (auto [elem_name, elem_value] : Items(value)) {
        auto parsed = elem_value.template As<T>();
        parsed.SetName(std::move(elem_name));
        parsed_array.emplace_back(std::move(parsed));
    }
    return parsed_array;
}

}  // namespace yaml_config

USERVER_NAMESPACE_END
