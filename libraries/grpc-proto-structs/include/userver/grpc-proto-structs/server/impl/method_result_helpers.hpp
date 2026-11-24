#pragma once

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/server/impl/message_storage.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/utils/optional_ref.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::server::impl {

template <typename Response>
ugrpc::server::Result<proto_structs::traits::CompatibleMessageType<Response>> ConvertMethodResult(
    ugrpc::server::Result<Response>&& result,
    utils::OptionalRef<google::protobuf::Arena> arena
) {
    if (result.IsSuccess()) {
        using Message = proto_structs::traits::CompatibleMessageType<Response>;
        using Storage = ugrpc::server::impl::MessageStorage<proto_structs::traits::CompatibleMessageType<Response>>;
        auto storage = arena ? Storage::AllocateOnArena(*arena) : Storage{};
        auto& message = storage.Get();
        proto_structs::StructToMessage(result.ExtractResponse(), message);
        if (arena) {
            return ugrpc::server::Result<Message>::BorrowArenaResponse(message);
        }
        return std::move(message);
    }
    return result.ExtractErrorStatus();
}

template <typename Response>
ugrpc::server::StreamingResult<proto_structs::traits::CompatibleMessageType<Response>> ConvertMethodResult(
    ugrpc::server::StreamingResult<Response>&& result,
    utils::OptionalRef<google::protobuf::Arena> arena
) {
    if (result.IsSuccess() && result.HasLastResponse()) {
        using Message = proto_structs::traits::CompatibleMessageType<Response>;
        using Storage = ugrpc::server::impl::MessageStorage<Message>;
        auto storage = arena ? Storage::AllocateOnArena(*arena) : Storage{};
        auto& message = storage.Get();
        proto_structs::StructToMessage(result.ExtractLastResponse(), message);
        if (arena) {
            return ugrpc::server::StreamingResult<Message>::BorrowArenaResponse(message);
        }
        return std::move(message);
    }
    return result.ExtractStatus();
}

}  // namespace grpc_proto_structs::server::impl

USERVER_NAMESPACE_END
