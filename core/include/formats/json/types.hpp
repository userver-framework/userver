#pragma once

#include <memory>
#include <vector>

#include <json/value.h>

#include <formats/common/path.hpp>

namespace formats {
namespace json {

using formats::common::Path;
using formats::common::PathToString;

enum class Type {
  kNull,
  kArray,
  kObject,
};

Json::ValueType ToNativeType(Type type);

using NativeValuePtr = std::shared_ptr<Json::Value>;

}  // namespace json
}  // namespace formats
