#pragma once

#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

/// @brief Read YAML from file
formats::yaml::Value FromFile(const std::string& path);

}  // namespace formats::yaml

USERVER_NAMESPACE_END
