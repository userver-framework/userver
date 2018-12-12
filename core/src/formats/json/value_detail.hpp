#pragma once

#include <formats/json/value.hpp>

namespace formats {
namespace json {

namespace detail {

class Value : public formats::json::Value {
 public:
  Value(NativeValuePtr&& root) noexcept
      : formats::json::Value(std::move(root)) {}

  using formats::json::Value::Get;
  using formats::json::Value::IsRoot;
};

// For testing purposes only
inline const Json::Value* GetPtr(const formats::json::Value& val) {
  return &static_cast<const Value&>(val).Get();
}

}  // namespace detail

}  // namespace json
}  // namespace formats
