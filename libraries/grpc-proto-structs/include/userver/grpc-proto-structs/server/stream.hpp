#pragma once

/// @file userver/grpc-proto-structs/server/stream.hpp
/// @brief proto-struct based server streaming interfaces

#include <utility>

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/server/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::server {

/// @brief proto-struct based Reader adapter
///
/// This class is not thread-safe.
///
/// If any method throws, further methods must not be called on the same stream.
/// @see @ref ugrpc::server::Reader.
template <typename Request>
class Reader {
public:
    using RequestMessage = proto_structs::traits::CompatibleMessageType<Request>;
    using ProtobufMessageReader = ugrpc::server::Reader<RequestMessage>;

    explicit Reader(ProtobufMessageReader& reader)
        : reader_{reader}
    {}

    /// @brief Await and read the next incoming message.
    ///
    /// Read protobuf message corresponding to `Request` with @ref ugrpc::server::Reader::Read
    /// and construct `Request` from it.
    ///
    /// @see @ref ugrpc::server::Reader::Read method for details.
    bool Read(Request& request) {
        RequestMessage message;
        if (reader_.Read(message)) {
            proto_structs::MessageToStruct(message, request);
            return true;
        }
        return false;
    }

private:
    ProtobufMessageReader& reader_;
};

/// @brief proto-struct based Writer adapter
///
/// This class is not thread-safe.
///
/// If any method throws, further methods must not be called on the same stream.
/// @see @ref ugrpc::server::Writer.
template <typename Response>
class Writer {
public:
    using ResponseMessage = proto_structs::traits::CompatibleMessageType<Response>;
    using ProtobufMessageWriter = ugrpc::server::Writer<ResponseMessage>;

    explicit Writer(ProtobufMessageWriter& writer)
        : writer_{writer}
    {}

    /// @{
    /// @brief Write the next outgoing message.
    /// @note This version may move some fields from the request.
    ///
    /// Convert response to corresponding protobuf message and pass it to @ref ugrpc::server::Writer::Write.
    ///
    /// @see @ref ugrpc::server::Writer::Write method for details.
    void Write(Response&& response) { writer_.Write(proto_structs::StructToMessage(std::move(response))); }

    void Write(Response&& response, const grpc::WriteOptions& options) {
        writer_.Write(proto_structs::StructToMessage(std::move(response)), options);
    }
    /// @}

    /// @{
    /// @brief Write the next outgoing message.
    /// @note This version preserves the original request object by copying necessary data.
    ///
    /// Convert response to corresponding protobuf message and pass it to @ref ugrpc::server::Writer::Write.
    ///
    /// @see @ref ugrpc::server::Writer::Write method for details.
    void WriteCopy(const Response& response) { writer_.Write(proto_structs::StructToMessage(response)); }

    void WriteCopy(const Response& response, const grpc::WriteOptions& options) {
        writer_.Write(proto_structs::StructToMessage(response), options);
    }
    /// @}

private:
    ProtobufMessageWriter& writer_;
};

/// @brief proto-struct based ReaderWriter adapter
///
/// If any method throws, further methods must not be called on the same stream.
///
/// This class allows the following concurrent calls:
///
///   - `Read`;
///   - `Write`
///
/// and there can only be one Read and one Write in flight at a time.
///
/// If any method throws, further methods must not be called on the same stream.
/// @see @ref ugrpc::server::ReaderWriter.
template <typename Request, typename Response>
class ReaderWriter : public Reader<Request>, public Writer<Response> {
public:
    using ProtobufMessageReaderWriter = ugrpc::server::ReaderWriter<
        typename Reader<Request>::RequestMessage,
        typename Writer<Response>::ResponseMessage>;

    explicit ReaderWriter(ProtobufMessageReaderWriter& reader_writer)
        : Reader<Request>{reader_writer},
          Writer<Response>{reader_writer}
    {}
};

}  // namespace grpc_proto_structs::server

USERVER_NAMESPACE_END
