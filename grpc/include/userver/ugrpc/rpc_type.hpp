#pragma once

/// @file userver/ugrpc/rpc_type.hpp
/// @brief @copybrief ugrpc::RpcType

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// Type categorizes RPCs by unary or streaming type
enum class RpcType {
    kUnary,
    kClientStreaming,
    kServerStreaming,
    kBidiStreaming,
};

/// Returns `true` for `RpcType::kUnary` and `RpcType::kServerStreaming`, where the client sends a single request
constexpr bool IsSingleRequestMethod(RpcType rpc_type) noexcept {
    return RpcType::kUnary == rpc_type || RpcType::kServerStreaming == rpc_type;
}

/// Returns `true` for `RpcType::kUnary` and `RpcType::kClientStreaming`, where the server sends back a single response
constexpr bool IsSingleResponseMethod(RpcType rpc_type) noexcept {
    return RpcType::kUnary == rpc_type || RpcType::kClientStreaming == rpc_type;
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
