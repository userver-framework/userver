#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

bool ParseEnforceTaskDeadline(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseEnforceTaskDeadline>
    kEnforceClientTaskDeadline;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
