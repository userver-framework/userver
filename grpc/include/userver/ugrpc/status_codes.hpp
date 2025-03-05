#pragma once

/// @file userver/ugrpc/status_codes.hpp
/// @brief Utilities for grpc::StatusCode

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @brief Convert string to grpc::StatusCode
/// @throws std::runtime_error
grpc::StatusCode StatusCodeFromString(std::string_view str);

/// @brief Convert grpc::StatusCode to string
std::string ToString(grpc::StatusCode code) noexcept;

/// @brief Whether a given status code is definitely a server-side error
///
/// Currently includes:
///
/// * `UNKNOWN`
/// * `UNIMPLEMENTED`
/// * `INTERNAL`
/// * `UNAVAILABLE`
/// * `DATA_LOSS`
///
/// We intentionally do not include `CANCELLED` and `DEADLINE_EXPIRED` here, because the situation may either be
/// considered not erroneous at all (when a client explicitly cancels an RPC; when a client attempts an RPC with a very
/// short deadline), or there is no single obvious service to blame (when the collective deadline expires for an RPC
/// tree).
bool IsServerError(grpc::StatusCode code) noexcept;

}  // namespace ugrpc

USERVER_NAMESPACE_END
