#pragma once

#include <memory>
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

  static VariableMap ParseFromFile(const std::string& path);

 private:
  Json::Value json_;
};

using VariableMapPtr = std::shared_ptr<VariableMap>;

}  // namespace json_config
