#include <json2yaml.hpp>

#include <string>

#include <userver/formats/common/conversion_stack.hpp>

/// [json2yaml - convert cpp]
USERVER_NAMESPACE_BEGIN

namespace formats::parse {

yaml::Value Convert(const json::Value& json, To<yaml::Value>) {
    return formats::common::PerformMinimalFormatConversion<yaml::Value>(json);
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
/// [json2yaml - convert cpp]
