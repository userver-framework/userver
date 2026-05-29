#pragma once

/// @file userver/ugrpc/server/middlewares/access_log/log_extra.hpp
/// @brief @copybrief ugrpc::server::middlewares::access_log::SetAdditionalLogKeys

#include <userver/logging/log_extra.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::access_log {

/// @brief Adds or extends log extra fields for gRPC access logging
/// @param context Middleware call context containing the storage
/// @param log_extra Additional log fields to add (will be moved from)
/// @snippet grpc/tests/logging/access_log_test.cpp grpc log extra tag
///
/// If log extra fields already exist in the context, extends them with new fields.
/// Otherwise, creates new log extra storage with the provided fields.
void SetAdditionalLogKeys(MiddlewareCallContext& context, logging::LogExtra&& log_extra);

}  // namespace ugrpc::server::middlewares::access_log

USERVER_NAMESPACE_END
