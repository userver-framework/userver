#pragma once

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

extern const dynamic_config::Key<bool> kLogRequest;

extern const dynamic_config::Key<bool> kLogRequestHeaders;

extern const dynamic_config::Key<bool> kCancelHandleRequestByDeadline;

struct CcCustomStatus final {
  http::HttpStatus initial_status_code;
  std::chrono::milliseconds max_time_delta;
};

CcCustomStatus Parse(const formats::json::Value& value,
                     formats::parse::To<CcCustomStatus>);

extern const dynamic_config::Key<CcCustomStatus> kCcCustomStatus;

extern const dynamic_config::Key<bool> kStreamApiEnabled;

}  // namespace server::handlers

USERVER_NAMESPACE_END
