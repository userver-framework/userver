/// [json2yaml - includes]
#pragma once

#include <userver/formats/json.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/yaml.hpp>
/// [json2yaml - includes]

/// [json2yaml - convert hpp]
USERVER_NAMESPACE_BEGIN

namespace formats::parse {

yaml::Value Convert(const json::Value& json, To<yaml::Value>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
/// [json2yaml - convert hpp]
