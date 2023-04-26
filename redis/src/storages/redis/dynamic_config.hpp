#pragma once

#include <chrono>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

bool ParseDeadlinePropagationEnabled(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseDeadlinePropagationEnabled>
    kDeadlinePropagationEnabled;

}  // namespace redis

USERVER_NAMESPACE_END
