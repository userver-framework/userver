#pragma once

#include <grpcpp/support/status_code_enum.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/*
  Retryable Status Codes:

   - grpc::StatusCode::CANCELLED
   - grpc::StatusCode::UNKNOWN
   - grpc::StatusCode::DEADLINE_EXCEEDED
   - grpc::StatusCode::ABORTED
   - grpc::StatusCode::INTERNAL
   - grpc::StatusCode::UNAVAILABLE
 */
bool IsRetryable(grpc::StatusCode status_code) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
