#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

bool ParseDeadlinePropagationEnabled(const dynamic_config::DocsMap&);

constexpr dynamic_config::Key<ParseDeadlinePropagationEnabled>
    kDeadlinePropagationEnabled;

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
