#pragma once

USERVER_NAMESPACE_BEGIN

namespace clients::http {

struct EnforceTaskDeadlineConfig {
  bool cancel_request{false};
  bool update_timeout{false};
};

}  // namespace clients::http

USERVER_NAMESPACE_END
