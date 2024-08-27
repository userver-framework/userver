#include <userver/ugrpc/byte_buffer_utils.hpp>

#include <fmt/format.h>
#include <grpcpp/support/proto_buffer_reader.h>
#include <grpcpp/support/proto_buffer_writer.h>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/numeric_cast.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

grpc::ByteBuffer SerializeToByteBuffer(
    const ::google::protobuf::Message& message, std::size_t block_size) {
  grpc::ByteBuffer buffer;
  // NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.UninitializedObject)
  grpc::ProtoBufferWriter writer{
      &buffer,
      /*block_size*/ utils::numeric_cast<int>(block_size),
      /*total_size*/ utils::numeric_cast<int>(message.ByteSizeLong()),
  };
  const bool success = message.SerializeToZeroCopyStream(&writer);
  if (!success) {
    throw std::runtime_error(
        fmt::format("Failed to serialize Protobuf message of type {}",
                    message.GetTypeName()));
  }
  return buffer;
}

bool ParseFromByteBuffer(grpc::ByteBuffer&& buffer,
                         ::google::protobuf::Message& message) {
  grpc::ProtoBufferReader reader{&buffer};
  return message.ParseFromZeroCopyStream(&reader);
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
