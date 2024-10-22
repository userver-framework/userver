#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief RPCs kinds
enum class CallKind {
    kUnaryCall,
    kInputStream,
    kOutputStream,
    kBidirectionalStream,
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
