#pragma once

#include <grpcpp/support/status.h>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Add required tags to the given span. It is expected that this span is
/// a call span from either server method or client method invocations.
/// If status code is not OK, then raise logging level to WARNING
void UpdateSpanWithStatus(tracing::Span& span, const grpc::Status& status);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
