#pragma once

#include <string>

#include <userver/compiler/demangle.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

[[noreturn]] void ThrowInvalidSecdistType(const formats::json::Value& val,
                                          const std::string& type);

std::string GetString(const formats::json::Value& parent_val,
                      const std::string& name);

int GetInt(const formats::json::Value& parent_val, const std::string& name,
           int dflt);

template <typename T>
T GetValue(const formats::json::Value& parent_val, const std::string& key,
           const T& dflt) {
  const auto& val = parent_val[key];
  try {
    return val.template As<T>(dflt);
  } catch (const formats::json::TypeMismatchException&) {
    ThrowInvalidSecdistType(val, compiler::GetTypeName<T>());
  }
}

void CheckIsObject(const formats::json::Value& val, const std::string& name);

void CheckIsArray(const formats::json::Value& val, const std::string& name);

}  // namespace storages::secdist

USERVER_NAMESPACE_END
