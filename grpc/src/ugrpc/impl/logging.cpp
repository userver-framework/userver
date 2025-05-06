#include <ugrpc/impl/logging.hpp>

#include <memory>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/status_utils.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const std::string kComponentTag{"grpc_component"};
const std::string kTypeTag{"grpc_type"};

std::string GetMessageForLogging(const google::protobuf::Message& message, MessageLoggingOptions options) {
    if (!logging::ShouldLog(options.log_level)) {
        return "hidden by log level";
    }

    if (!options.trim_secrets || !HasSecrets(message)) {
        return ugrpc::impl::ToLimitedString(message, options.max_size);
    }

    std::unique_ptr<google::protobuf::Message> trimmed{message.New()};
    trimmed->CopyFrom(message);
    TrimSecrets(*trimmed);
    return ugrpc::impl::ToLimitedString(*trimmed, options.max_size);
}

std::string GetErrorDetailsForLogging(const grpc::Status& status) {
    if (status.ok()) {
        return {};
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
