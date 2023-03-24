#pragma once

#include <string>
#include <vector>

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace logging {

struct DynamicDebugConfig {
  std::vector<std::string> force_enabled;
  std::vector<std::string> force_disabled;
};

bool operator==(const DynamicDebugConfig& a, const DynamicDebugConfig& b);

DynamicDebugConfig Parse(const formats::json::Value&,
                         formats::parse::To<DynamicDebugConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
