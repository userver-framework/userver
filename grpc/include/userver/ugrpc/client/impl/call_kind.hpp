#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// @brief RPCs kinds
enum class CallKind {
    kUnaryCall,
    kInputStream,
    kOutputStream,
    kBidirectionalStream,
};

constexpr bool IsClientStreaming(CallKind kind) noexcept {
    return CallKind::kOutputStream == kind || CallKind::kBidirectionalStream == kind;
}

constexpr bool IsServerStreaming(CallKind kind) noexcept {
    return CallKind::kInputStream == kind || CallKind::kBidirectionalStream == kind;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
