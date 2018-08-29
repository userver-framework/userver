#include "helpers.hpp"

#include <iostream>
#include <sstream>

#include <json/value.h>

#include "exceptions.hpp"

namespace storages {
namespace secdist {

void ThrowInvalidSecdistType(const std::string& name, const std::string& type) {
  throw InvalidSecdistJson('\'' + name + "' is not " + type +
                           " (or not found)");
}

std::string GetString(const Json::Value& parent_val, const std::string& name) {
  const auto& val = parent_val[name];
  if (!val.isString()) {
    ThrowInvalidSecdistType(name, "a string");
  }
  return val.asString();
}

int GetInt(const Json::Value& parent_val, const std::string& name, int dflt) {
  const auto& val = parent_val[name];
  if (val.isNull()) {
    return dflt;
  }
  if (!val.isInt()) {
    ThrowInvalidSecdistType(name, "an int");
  }
  return val.asInt();
}

void CheckIsObject(const Json::Value& val, const std::string& name) {
  if (!val.isObject()) {
    ThrowInvalidSecdistType(name, "an object");
  }
}

void CheckIsArray(const Json::Value& val, const std::string& name) {
  if (!val.isArray()) {
    ThrowInvalidSecdistType(name, "an array");
  }
}

}  // namespace secdist
}  // namespace storages
