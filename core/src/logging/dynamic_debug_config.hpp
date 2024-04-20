#pragma once

#include <string>
#include <unordered_map>

#include <userver/formats/parse/to.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace logging {

struct DynamicDebugConfig {
  std::unordered_map<std::string, logging::Level> force_enabled;
  std::unordered_map<std::string, logging::Level> force_disabled;
};

bool operator==(const DynamicDebugConfig& a, const DynamicDebugConfig& b);

DynamicDebugConfig Parse(const formats::json::Value&,
                         formats::parse::To<DynamicDebugConfig>);

}  // namespace logging

USERVER_NAMESPACE_END
