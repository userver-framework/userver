#pragma once

#include <string>

#include <formats/json/value.hpp>

namespace storages {
namespace secdist {

void ThrowInvalidSecdistType(const std::string& name, const std::string& type);

std::string GetString(const formats::json::Value& parent_val,
                      const std::string& name);

int GetInt(const formats::json::Value& parent_val, const std::string& name,
           int dflt);

void CheckIsObject(const formats::json::Value& val, const std::string& name);

void CheckIsArray(const formats::json::Value& val, const std::string& name);

}  // namespace secdist
}  // namespace storages
