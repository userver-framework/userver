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
std::string_view ToString(grpc::StatusCode code) noexcept;

}  // namespace ugrpc

USERVER_NAMESPACE_END
