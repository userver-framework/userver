#pragma once

#include <string>

namespace Json {
class Value;
}

namespace storages {
namespace secdist {

void ThrowInvalidSecdistType(const std::string& name, const std::string& type);

std::string GetString(const Json::Value& parent_val, const std::string& name);

int GetInt(const Json::Value& parent_val, const std::string& name, int dflt);

void CheckIsObject(const Json::Value& val, const std::string& name);

void CheckIsArray(const Json::Value& val, const std::string& name);

}  // namespace secdist
}  // namespace storages
