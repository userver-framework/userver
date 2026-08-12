#pragma once

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// Versions of grpc before https://github.com/grpc/grpc/commit/ee6464083ffd08c93ef10dae0bedbb0837aca0ec
// might return non-standard error status code. This can lead to undefined behavior when interacting
// with the `grpc::StatusCode` enum type. To address this, any status values that fall outside the
// [kMinValue, kMaxValue] range are mapped to grpc::StatusCode::UNKNOWN.
// This aligns with specification which only permits codes 0-16.
void ClampStatusCodeToValidRange(grpc::Status& status);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
