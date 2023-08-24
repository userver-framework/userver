#include <userver/testsuite/grpc_control.hpp>

#include <algorithm>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

GrpcControl::GrpcControl(std::chrono::milliseconds timeout, bool is_tls_enabled)
    : timeout_(timeout), is_tls_enabled_(is_tls_enabled) {}

std::chrono::milliseconds GrpcControl::MakeTimeout(
    std::chrono::milliseconds duration) const {
  return std::max(duration, timeout_);
}

bool GrpcControl::IsTlsEnabled() const { return is_tls_enabled_; }

}  // namespace testsuite

USERVER_NAMESPACE_END
