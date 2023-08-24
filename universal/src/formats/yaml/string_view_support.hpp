#pragma once

#include <string_view>

#include <yaml-cpp/yaml.h>

namespace YAML {

// Makes rapidjson::GenericValue accept std::string_view as keys
template <>
struct convert<std::string_view> {
  static Node encode(const std::string_view& rhs) {
    return Node(std::string{rhs});
  }

  static bool decode(const Node& node, std::string_view& rhs) {
    if (!node.IsScalar()) return false;
    rhs = node.Scalar();
    return true;
  }
};

}  // namespace YAML
