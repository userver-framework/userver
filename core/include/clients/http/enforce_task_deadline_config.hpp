#pragma once

namespace clients::http {

struct EnforceTaskDeadlineConfig {
  bool cancel_request{false};
  bool update_timeout{false};
};

}  // namespace clients::http
