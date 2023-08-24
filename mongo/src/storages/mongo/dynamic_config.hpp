#pragma once

#include <chrono>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

std::chrono::milliseconds ParseDefaultMaxTime(
    const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseDefaultMaxTime> kDefaultMaxTime;

bool ParseDeadlinePropagationEnabled(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseDeadlinePropagationEnabled>
    kDeadlinePropagationEnabled;

bool ParseCongestionControlEnabled(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseCongestionControlEnabled>
    kCongestionControlEnabled;

}  // namespace storages::mongo

USERVER_NAMESPACE_END
