#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

bool ParseLogRequest(const dynamic_config::DocsMap&);

inline constexpr dynamic_config::Key<ParseLogRequest> kLogRequest;

bool ParseLogRequestHeaders(const dynamic_config::DocsMap&);

inline constexpr dynamic_config::Key<ParseLogRequestHeaders> kLogRequestHeaders;

bool ParseCheckAuthInHandlers(const dynamic_config::DocsMap&);

inline constexpr dynamic_config::Key<ParseCheckAuthInHandlers>
    kCheckAuthInHandlers;

bool ParseCancelHandleRequestByDeadline(const dynamic_config::DocsMap&);

inline constexpr dynamic_config::Key<ParseCancelHandleRequestByDeadline>
    kCancelHandleRequestByDeadline;

}  // namespace server::handlers

USERVER_NAMESPACE_END
