#pragma once

#include <utility>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

class YamlConfig;

template <typename T>
std::vector<T> ParseMapToArray(const YamlConfig& value) {
  value.CheckObjectOrNull();
  std::vector<T> parsed_array;
  parsed_array.reserve(value.GetSize());

  for (const auto& [elem_name, elem_value] : Items(value)) {
    auto parsed = elem_value.template As<T>();
    parsed.SetName(elem_name);
    parsed_array.emplace_back(std::move(parsed));
  }
  return parsed_array;
}

}  // namespace yaml_config

USERVER_NAMESPACE_END
