#pragma once

#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

void Merge(Schema& destination, Schema&& source);

}  // namespace yaml_config

USERVER_NAMESPACE_END
