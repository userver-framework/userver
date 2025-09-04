#include <ugrpc/impl/logging.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/status_utils.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const std::string kBodyTag{"body"};
const std::string kCodeTag{"grpc_code"};
const std::string kComponentTag{"grpc_component"};
const std::string kMessageMarshalledLenTag{"grpc_message_marshalled_len"};
const std::string kTypeTag{"grpc_type"};

std::string GetMessageForLogging(const google::protobuf::Message& message, std::size_t max_size) {
    return ugrpc::impl::ToLimitedDebugString(message, max_size);
}

std::string GetErrorDetailsForLogging(const grpc::Status& status) {
    if (status.ok()) {
        return "";
    }

    const auto gstatus = ugrpc::ToGoogleRpcStatus(status);
    return gstatus.has_value()
               ? fmt::format(
                     "code: {}, error message: {}\nerror details:\n{}",
                     ugrpc::ToString(status.error_code()),
                     status.error_message(),
                     ugrpc::GetGStatusLimitedMessage(*gstatus)
                 )
               : fmt::format(
                     "code: {}, error message: {}", ugrpc::ToString(status.error_code()), status.error_message()
                 );
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
