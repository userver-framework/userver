#pragma once

#include <memory>
#include <string>

#include <formats/json/value.hpp>

namespace json_config {

class VariableMap {
 public:
  VariableMap();
  explicit VariableMap(formats::json::Value json);

  const formats::json::Value& Json() const { return json_; }

  bool IsDefined(const std::string& name) const;
  formats::json::Value GetVariable(const std::string& name) const;

  static VariableMap ParseFromFile(const std::string& path);

 private:
  formats::json::Value json_;
};

using VariableMapPtr = std::shared_ptr<VariableMap>;

}  // namespace json_config
