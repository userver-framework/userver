#pragma once

#include <chrono>
#include <unordered_set>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/http/http_status.hpp>

#include <dynamic_config/variables/USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE.hpp>
#include <dynamic_config/variables/USERVER_LOG_REQUEST.hpp>
#include <dynamic_config/variables/USERVER_LOG_REQUEST_HEADERS.hpp>
#include <dynamic_config/variables/USERVER_LOG_REQUEST_HEADERS_WHITELIST.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

using HeadersWhitelist = std::unordered_set<std::string>;

}  // namespace server::handlers

USERVER_NAMESPACE_END
