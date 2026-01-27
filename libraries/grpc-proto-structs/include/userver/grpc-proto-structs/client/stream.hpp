#pragma once

/// @file
/// @brief Structs Reader/Writer stream wrappers over Vanilla streams.

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/client/stream.hpp>
#include <userver/utils/box.hpp>

#include <userver/grpc-proto-structs/client/stream_read_future.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::client {

/// @brief proto-struct based Reader adapter.
template <typename StructsResponse>
class Reader final {
public:
    using ProtobufResponse = proto_structs::traits::CompatibleMessageType<StructsResponse>;
    using ProtobufReader = ugrpc::client::Reader<ProtobufResponse>;

    explicit Reader(ProtobufReader&& reader)
        : reader_{std::move(reader)}
    {}

    Reader(Reader&&) = default;
    Reader& operator=(Reader&&) = default;

    /// @brief Await and read the next incoming message.
    ///
    /// Read protobuf message corresponding to Response with @ref ugrpc::client::Reader::Read
    /// and construct response from it.
    [[nodiscard]] std::optional<StructsResponse> Read() {
        ProtobufResponse message;
        if (reader_.Read(message)) {
            StructsResponse response;
            proto_structs::MessageToStruct(message, response);
            return {response};
        }
        return std::nullopt;
        ;
    }

    /// @brief Get call context, useful e.g. for accessing metadata.
    ///
    /// @see @ref ugrpc::client::Reader::GetContext.
    ugrpc::client::CallContext& GetContext() { return reader_.GetContext(); }

    /// @overload
    ///
    /// @see @ref ugrpc::client::Reader::GetContext.
    const ugrpc::client::CallContext& GetContext() const { return reader_.GetContext(); }

private:
    ProtobufReader reader_;
};

/// @brief proto-struct based Writer adapter.
template <typename StructsRequest, typename StructsResponse>
class Writer final {
public:
    using ProtobufResponse = proto_structs::traits::CompatibleMessageType<StructsResponse>;
    using ProtobufRequest = proto_structs::traits::CompatibleMessageType<StructsRequest>;

    using ProtobufWriter = ugrpc::client::Writer<ProtobufRequest, ProtobufResponse>;

    explicit Writer(ProtobufWriter&& writer)
        : writer_{std::move(writer)}
    {}

    Writer(Writer&&) = default;
    Writer& operator=(Writer&&) = default;

    /// @brief Write the next outgoing message.
    /// @note This version may move some fields from the request.
    [[nodiscard]] bool Write(StructsRequest&& request) {
        return writer_.Write(proto_structs::StructToMessage(std::move(request)));
    }

    /// @brief Write the next outgoing message.
    /// @note This version preserves the original request object by copying necessary data.
    [[nodiscard]] bool WriteCopy(const StructsRequest& request) {
        return writer_.Write(proto_structs::StructToMessage(request));
    }

    /// @brief Write the next outgoing message and check result.
    /// @note This version may move some fields from the request.
    void WriteAndCheck(StructsRequest&& request) {
        writer_.WriteAndCheck(proto_structs::StructToMessage(std::move(request)));
    }

    /// @brief Write the next outgoing message and check result.
    /// @note This version preserves the original request object by copying necessary data.
    void WriteCopyAndCheck(const StructsRequest& request) {
        writer_.WriteAndCheck(proto_structs::StructToMessage(request));
    }

    /// @brief Complete the RPC successfully
    StructsResponse Finish() { return proto_structs::MessageToStruct<StructsResponse>(writer_.Finish()); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    ugrpc::client::CallContext& GetContext() { return writer_.GetContext(); }

    /// @overload
    const ugrpc::client::CallContext& GetContext() const { return writer_.GetContext(); }

private:
    ProtobufWriter writer_;
};

/// @brief proto-struct based ReaderWriter adapter.
template <typename StructsRequest, typename StructsResponse>
class ReaderWriter final {
public:
    using ProtobufResponse = proto_structs::traits::CompatibleMessageType<StructsResponse>;
    using ProtobufRequest = proto_structs::traits::CompatibleMessageType<StructsRequest>;

    using ProtobufReaderWriter = ugrpc::client::ReaderWriter<ProtobufRequest, ProtobufResponse>;

    explicit ReaderWriter(ProtobufReaderWriter&& reader_writer)
        : reader_writer_{std::move(reader_writer)}
    {}

    ReaderWriter(ReaderWriter&&) = default;
    ReaderWriter& operator=(ReaderWriter&&) = default;

    /// @brief Await and read the next incoming message.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::Read.
    [[nodiscard]] std::optional<StructsResponse> Read() {
        ProtobufResponse message;
        if (reader_writer_.Read(message)) {
            StructsResponse response;
            proto_structs::MessageToStruct(message, response);
            return {response};
        }
        return std::nullopt;
    }

    /// @brief Return future to read next incoming result.
    StreamReadFuture<StructsResponse> ReadAsync() {
        return StreamReadFuture{reader_writer_.ReadAsync(*response_), *response_};
    }

    /// @brief Write the next outgoing message.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::Write.
    [[nodiscard]] bool Write(const StructsRequest& request) {
        return reader_writer_.Write(proto_structs::StructToMessage(request));
    }

    /// @brief Write the next outgoing message.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::Write.
    [[nodiscard]] bool Write(StructsRequest&& request) {
        return reader_writer_.Write(proto_structs::StructToMessage(std::move(request)));
    }

    /// @brief Write the next outgoing message and check result.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::WriteAndCheck.
    void WriteAndCheck(const StructsRequest& request) {
        reader_writer_.WriteAndCheck(proto_structs::StructToMessage(request));
    }

    /// @brief Write the next outgoing message and check result.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::WriteAndCheck.
    void WriteAndCheck(StructsRequest&& request) {
        reader_writer_.WriteAndCheck(proto_structs::StructToMessage(std::move(request)));
    }

    /// @brief Announce end-of-output to the server.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::WritesDone.
    [[nodiscard]] bool WritesDone() { return reader_writer_.WritesDone(); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    ///
    /// @see @ref ugrpc::client::ReaderWriter::GetContext.
    ugrpc::client::CallContext& GetContext() { return reader_writer_.GetContext(); }

    /// @overload
    ///
    /// @see @ref ugrpc::client::ReaderWriter::GetContext.
    const ugrpc::client::CallContext& GetContext() const { return reader_writer_.GetContext(); }

private:
    ProtobufReaderWriter reader_writer_;
    utils::Box<ProtobufResponse> response_;
};

}  // namespace grpc_proto_structs::client

USERVER_NAMESPACE_END
