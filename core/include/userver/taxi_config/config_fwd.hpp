#pragma once

#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

using Snapshot = dynamic_config::Snapshot;
using Source = dynamic_config::Source;
using DocsMap = dynamic_config::DocsMap;
using KeyValue = dynamic_config::KeyValue;

}  // namespace taxi_config

namespace components {

using TaxiConfig = DynamicConfig;

}  // namespace components

USERVER_NAMESPACE_END
