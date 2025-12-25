#pragma once

#include <grpcpp/support/status.h>

#include <userver/server/handlers/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

// The gRPC specification (https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md#responses)
// requires status-messages to be percent-encoded but allows spaces (0x20) to remain unencoded.
// However, this conflicts with HTTP/2 RFC9113 which strictly forbids leading and trailing whitespace (0x20, 0x09)
// in header field values (https://datatracker.ietf.org/doc/html/rfc9113#section-8.2.1-3.4).
// So we need to trim whitespaces from gRPC status messages before transmission to ensure compliance with HTTP/2.
void TrimStatusErrorMessage(grpc::Status& status);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
