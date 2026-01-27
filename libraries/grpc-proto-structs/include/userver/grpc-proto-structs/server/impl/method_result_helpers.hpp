#pragma once

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/server/result.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::server::impl {

template <typename Response>
ugrpc::server::Result<proto_structs::traits::CompatibleMessageType<Response>> ConvertMethodResult(
    ugrpc::server::Result<Response>&& result
) {
    if (result.IsSuccess()) {
        return proto_structs::StructToMessage(result.ExtractResponse());
    }
    return result.ExtractErrorStatus();
}

template <typename Response>
ugrpc::server::StreamingResult<proto_structs::traits::CompatibleMessageType<Response>> ConvertMethodResult(
    ugrpc::server::StreamingResult<Response>&& result
) {
    if (result.IsSuccess() && result.HasLastResponse()) {
        return proto_structs::StructToMessage(result.ExtractLastResponse());
    }
    return result.ExtractStatus();
}

}  // namespace grpc_proto_structs::server::impl

USERVER_NAMESPACE_END
