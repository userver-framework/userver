#include <userver/ugrpc/status_utils.hpp>

#include <fmt/format.h>
#include <cstddef>

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include <google/rpc/status.pb.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

constexpr std::size_t kMessageLengthLimit = 1024;

}  // namespace

grpc::Status ToGrpcStatus(const google::rpc::Status& gstatus) {
    grpc::StatusCode code = grpc::StatusCode::UNKNOWN;
    if (gstatus.code() >= grpc::StatusCode::OK && gstatus.code() <= grpc::StatusCode::UNAUTHENTICATED) {
        code = static_cast<grpc::StatusCode>(gstatus.code());
    }
    return grpc::Status(code, gstatus.message(), gstatus.SerializeAsString());
}

std::optional<google::rpc::Status> ToGoogleRpcStatus(const grpc::Status& status) {
    if (status.error_details().empty()) {
        return {};
    }
    google::rpc::Status gstatus;
    if (!gstatus.ParseFromString(status.error_details())) {
        return {};
    }
    return gstatus;
}

std::string GetGStatusLimitedMessage(const google::rpc::Status& status) {
    std::string message(kMessageLengthLimit, '\0');
    google::protobuf::io::ArrayOutputStream stream(message.data(), kMessageLengthLimit);
    google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.SetExpandAny(true);

    printer.Print(status, &stream);
    message.resize(std::min(static_cast<std::size_t>(stream.ByteCount()), kMessageLengthLimit));
    // Single line mode currently might have an extra space at the end.
    if (!message.empty() && message[message.size() - 1] == ' ') {
        message.resize(message.size() - 1);
    }
    return message;
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
