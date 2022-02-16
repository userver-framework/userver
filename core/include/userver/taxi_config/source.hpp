#pragma once

#include <userver/dynamic_config/source.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

template <auto Parser>
using Key = dynamic_config::Key<Parser>;

using Source = dynamic_config::Source;
using Snapshot = dynamic_config::Snapshot;
using DocsMap = dynamic_config::DocsMap;

}  // namespace taxi_config

USERVER_NAMESPACE_END
