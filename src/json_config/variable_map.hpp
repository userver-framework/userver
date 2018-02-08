#pragma once

#include <string>

#include <json/value.h>

namespace json_config {

class VariableMap {
 public:
  VariableMap();
  explicit VariableMap(Json::Value json);

  const Json::Value& Json() const { return json_; }

  bool IsDefined(const std::string& name) const;
  const Json::Value& GetVariable(const std::string& name) const;

 private:
  Json::Value json_;
};

}  // namespace json_config
