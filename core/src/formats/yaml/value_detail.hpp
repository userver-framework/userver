#pragma once

#include <formats/yaml/value.hpp>

namespace formats {
namespace yaml {

namespace detail {

class Value : public formats::yaml::Value {
 public:
  Value(YAML::Node&& root) noexcept : formats::yaml::Value(std::move(root)) {}

  using formats::yaml::Value::Get;
  using formats::yaml::Value::IsRoot;
  using formats::yaml::Value::IsSameNode;
};

// For testing purposes only
inline bool IsSamePtrs(const formats::yaml::Value& lhs,
                       const formats::yaml::Value& rhs) {
  return static_cast<const Value&>(lhs).IsSameNode(rhs);
}

}  // namespace detail

}  // namespace yaml
}  // namespace formats
