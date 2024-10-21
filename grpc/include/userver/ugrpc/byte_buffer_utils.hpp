#pragma once

/// @file userver/ugrpc/byte_buffer_utils.hpp
/// @brief Helper functions for working with `grpc::ByteBuffer`

#include <cstddef>

#include <google/protobuf/message.h>
#include <grpcpp/support/byte_buffer.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @see @ref SerializeToByteBuffer
inline constexpr std::size_t kDefaultSerializeBlockSize = 4096;

/// @brief Serialize a Protobuf message to the wire format.
/// @param message the message to serialize
/// @param block_size can be used for performance tuning, too small chunk size
/// results in extra allocations, too large chunk size results in wasting memory
/// @throws std::runtime_error on serialization errors (supposedly only happens
/// on extremely rare allocation failures or proto reflection malfunctioning).
grpc::ByteBuffer
SerializeToByteBuffer(const ::google::protobuf::Message& message, std::size_t block_size = kDefaultSerializeBlockSize);

/// @brief Parse a Protobuf message from the wire format.
/// @param buffer the buffer that might be tempered with during deserialization
/// @param message will contain the parsing result on success
/// @returns `true` on success, `false` if @a buffer does not contain a valid
/// message, according to the derived type of @a message
bool ParseFromByteBuffer(grpc::ByteBuffer&& buffer, ::google::protobuf::Message& message);

}  // namespace ugrpc

USERVER_NAMESPACE_END
