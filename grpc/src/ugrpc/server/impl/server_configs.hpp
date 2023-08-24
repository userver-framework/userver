#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

bool ParseCancelTaskByDeadline(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseCancelTaskByDeadline>
    kServerCancelTaskByDeadline;

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
