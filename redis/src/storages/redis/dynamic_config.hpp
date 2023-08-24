#pragma once

#include <chrono>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

int ParseDeadlinePropagationVersion(const dynamic_config::DocsMap& docs_map);
inline constexpr dynamic_config::Key<ParseDeadlinePropagationVersion>
    kDeadlinePropagationVersion;
inline constexpr int kDeadlinePropagationExperimentVersion = 1;

bool ParseRedisClusterAutoTopology(const dynamic_config::DocsMap& docs_map);
inline constexpr dynamic_config::Key<ParseRedisClusterAutoTopology>
    kRedisAutoTopologyEnabled;

}  // namespace redis

USERVER_NAMESPACE_END
