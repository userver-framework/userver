#pragma once

#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/support/byte_buffer.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

template <typename Message>
grpc::Status DeserializeMessage(grpc::ByteBuffer&& buffer, Message& message) {
    const auto status = grpc::SerializationTraits<Message>::Deserialize(&buffer, &message);
    buffer.Release();
    if (!status.ok()) {
        return {grpc::StatusCode::INTERNAL, "Unable to parse request"};
    }
    return grpc::Status::OK;
}

template <typename Message>
grpc::Status SerializeMessage(const Message& message, grpc::ByteBuffer& buffer) {
    bool own_buffer = false;
    return grpc::SerializationTraits<Message>::Serialize(message, &buffer, &own_buffer);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
