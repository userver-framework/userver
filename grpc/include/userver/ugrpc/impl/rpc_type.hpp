#pragma once

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

/// Type categorizes RPCs by unary or streaming type
enum class RpcType {
    kUnary,
    kClientStreaming,
    kServerStreaming,
    kBidiStreaming,
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
