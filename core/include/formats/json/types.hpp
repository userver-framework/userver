#pragma once

#include <memory>
#include <vector>

#include <json/value.h>

namespace formats {
namespace json {

using Path = std::vector<std::string>;

std::string PathToString(const Path& path, const std::string& root = "/",
                         char separator = '.');

enum class Type {
  kNull,
  kArray,
  kObject,
};

Json::ValueType ToNativeType(Type type);

using NativeValuePtr = std::shared_ptr<Json::Value>;

}  // namespace json
}  // namespace formats
