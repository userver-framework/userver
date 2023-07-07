#pragma once
// extended types to allow extract more precie type information from rapidjson's
// Value

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <userver/formats/json/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::impl {
enum Type {
  nullValue = rapidjson::kNullType,
  objectValue = rapidjson::kObjectType,
  arrayValue = rapidjson::kArrayType,
  stringValue = rapidjson::kStringType,
  booleanValue = 1000,  // make sure we don't declare overlapping values
  intValue,
  uintValue,
  realValue,
  errorValue
};

Type GetExtendedType(const Value& val);
const char* NameForType(Type expected);
}  // namespace formats::json::impl

USERVER_NAMESPACE_END
