#pragma once

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/storage/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

using TaxiConfig = DynamicConfig;

}  // namespace components

namespace taxi_config {

template <auto Parser>
using Key = dynamic_config::Key<Parser>;

using Snapshot = dynamic_config::Snapshot;
using DocsMap = dynamic_config::DocsMap;
using Source = dynamic_config::Source;

}  // namespace taxi_config

USERVER_NAMESPACE_END
