#include "helpers.hpp"

#include <iostream>
#include <sstream>

#include "exceptions.hpp"

namespace storages {
namespace secdist {

void ThrowInvalidSecdistType(const std::string& name, const std::string& type) {
  throw InvalidSecdistJson('\'' + name + "' is not " + type +
                           " (or not found)");
}

std::string GetString(const formats::json::Value& parent_val,
                      const std::string& name) {
  const auto& val = parent_val[name];
  if (!val.isString()) {
    ThrowInvalidSecdistType(name, "a string");
  }
  return val.asString();
}

int GetInt(const formats::json::Value& parent_val, const std::string& name,
           int dflt) {
  const auto& val = parent_val[name];
  if (val.isNull()) {
    return dflt;
  }
  if (!val.isInt()) {
    ThrowInvalidSecdistType(name, "an int");
  }
  return val.asInt();
}

void CheckIsObject(const formats::json::Value& val, const std::string& name) {
  if (!val.isObject()) {
    ThrowInvalidSecdistType(name, "an object");
  }
}

void CheckIsArray(const formats::json::Value& val, const std::string& name) {
  if (!val.isArray()) {
    ThrowInvalidSecdistType(name, "an array");
  }
}

}  // namespace secdist
}  // namespace storages
