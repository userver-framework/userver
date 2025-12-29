#pragma once

#include <optional>
#include <string_view>

#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {
struct ClientQos;
}  // namespace ugrpc::client

namespace ugrpc::client::impl {

/// @brief Enumeration of possible RPC path validation errors.
enum class RpcPathValidationError {
    kEmptyPath,
    kLeadingSlash,
    kMissingSlash,
    kTooManySlashes,
    kMissingDotInService,
    kEmptyMethod,
    kInvalidFormat,
    kNotFound
};

/// @brief Converts an RpcPathValidationError to a human-readable string.
std::string_view ToString(RpcPathValidationError error);

/// @brief Validates the format of an RPC method path.
/// @details An RPC path should be in the format: `path.to.ServiceName/MethodName`.
/// The special value `__default__` is also considered valid.
/// @param path The RPC method path to validate.
/// @return An optional containing an error code if the path is invalid, or `std::nullopt` if it is valid.
std::optional<RpcPathValidationError> IsValidRpcMethodPath(std::string_view path);

/// @brief Checks if a method belongs to a specific service.
/// @param method_path The full path of the method (e.g., `path.to.ServiceName/MethodName`).
/// @param service_name The full name of the service (e.g., `path.to.ServiceName`).
/// @return `true` if the method's service part matches the service name, `false` otherwise.
///         Returns `false` for the `__default__` method path.
bool IsMethodOfService(std::string_view method_path, std::string_view service_name);

enum class QosValidationResult { kOk, kHasErrors };

/// @brief Validates that methods specified in the client QOS configuration exist for the service.
/// @details Iterates through the methods in `client_qos` and checks:
/// 1. If the method path format is valid using `IsValidRpcMethodPath`.
/// 2. If the method belongs to the service described by `metadata`.
/// 3. If the method exists in the service's static metadata.
/// A warning is logged for each invalid or non-existent method.
/// @param client_qos The client QOS configuration.
/// @param metadata The static metadata for the gRPC service.
template <typename OnError>
QosValidationResult ValidateClientQosMethodsExistence(
    const ClientQos& client_qos,
    const ugrpc::impl::StaticServiceMetadata& metadata,
    OnError&& on_error
) {
    bool failed = false;
    for (const auto& [method_path, qos] : client_qos.methods) {
        if (const auto error = IsValidRpcMethodPath(method_path)) {
            on_error(method_path, *error);
            failed = true;
            continue;
        }

        if (!IsMethodOfService(method_path, metadata.service_full_name)) {
            // Method is for a different service - this is OK, just skip existence validation
            continue;
        }

        const auto method_id = FindMethod(metadata, method_path);
        if (!method_id.has_value()) {
            on_error(method_path, RpcPathValidationError::kNotFound);
            failed = true;
        }
    }
    return failed ? QosValidationResult::kHasErrors : QosValidationResult::kOk;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
