#include <userver/formats/parse/may_parse.hpp>

#include <string>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

namespace impl {

bool IsJsonNumber(const formats::json::Value& value) { return value.IsNumber(); }

}  // namespace impl

bool MayParseAs(const formats::json::Value& value, To<std::string>) { return value.IsString(); }

bool MayParseAs(const formats::json::Value& value, To<bool>) { return value.IsBool(); }

}  // namespace formats::parse

USERVER_NAMESPACE_END
