#pragma once

#include <string>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

[[noreturn]] void ThrowInvalidSecdistType(const std::string& name,
                                          const std::string& type);

std::string GetString(const formats::json::Value& parent_val,
                      const std::string& name);

int GetInt(const formats::json::Value& parent_val, const std::string& name,
           int dflt);

void CheckIsObject(const formats::json::Value& val, const std::string& name);

void CheckIsArray(const formats::json::Value& val, const std::string& name);

}  // namespace storages::secdist

USERVER_NAMESPACE_END
