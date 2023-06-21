#pragma once

#include <chrono>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

class GrpcControl {
 public:
  GrpcControl() = default;

  GrpcControl(std::chrono::milliseconds timeout, bool is_tls_enabled);

  std::chrono::milliseconds MakeTimeout(
      std::chrono::milliseconds duration) const;

  bool IsTlsEnabled() const;

 private:
  std::chrono::milliseconds timeout_{};
  bool is_tls_enabled_{false};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
