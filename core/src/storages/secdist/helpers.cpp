#include <userver/storages/secdist/helpers.hpp>

#include <iostream>
#include <sstream>

#include <userver/formats/json/exception.hpp>
#include <userver/storages/secdist/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

[[noreturn]] void ThrowInvalidSecdistType(const std::string& name,
                                          const std::string& type) {
  throw InvalidSecdistJson('\'' + name + "' is not " + type +
                           " (or not found)");
}

std::string GetString(const formats::json::Value& parent_val,
                      const std::string& name) {
  const auto& val = parent_val[name];
  if (!val.IsString()) {
    ThrowInvalidSecdistType(name, "a string");
  }
  return val.As<std::string>();
}

int GetInt(const formats::json::Value& parent_val, const std::string& name,
           int dflt) {
  try {
    return parent_val[name].As<int>();
  } catch (const formats::json::MemberMissingException&) {
    return dflt;
  } catch (const formats::json::TypeMismatchException&) {
    ThrowInvalidSecdistType(name, "an int");
  }
}

void CheckIsObject(const formats::json::Value& val, const std::string& name) {
  if (!val.IsObject()) {
    ThrowInvalidSecdistType(name, "an object");
  }
}

void CheckIsArray(const formats::json::Value& val, const std::string& name) {
  if (!val.IsArray()) {
    ThrowInvalidSecdistType(name, "an array");
  }
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
