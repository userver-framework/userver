#pragma once

#include <cstdint>
#include <string>

#include <json/value.h>
#include <boost/optional.hpp>

#include "variable_map.hpp"

namespace json_config {

class JsonConfig {
 public:
  JsonConfig(Json::Value json, std::string full_path,
             VariableMapPtr config_vars_ptr);

  const Json::Value& Json() const;
  const std::string& FullPath() const;
  const VariableMapPtr& ConfigVarsPtr() const;

  int ParseInt(const std::string& name) const;
  int ParseInt(const std::string& name, int dflt) const;
  boost::optional<int> ParseOptionalInt(const std::string& name) const;
  bool ParseBool(const std::string& name) const;
  bool ParseBool(const std::string& name, bool dflt) const;
  boost::optional<bool> ParseOptionalBool(const std::string& name) const;
  uint64_t ParseUint64(const std::string& name) const;
  uint64_t ParseUint64(const std::string& name, uint64_t dflt) const;
  boost::optional<uint64_t> ParseOptionalUint64(const std::string& name) const;
  std::string ParseString(const std::string& name) const;
  std::string ParseString(const std::string& name,
                          const std::string& dflt) const;
  boost::optional<std::string> ParseOptionalString(
      const std::string& name) const;

 private:
  Json::Value json_;
  std::string full_path_;
  VariableMapPtr config_vars_ptr_;
};

}  // namespace json_config
