#include <ugrpc/impl/logging.hpp>

#include <memory>

#include <userver/logging/log.hpp>
#include <userver/utils/log.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

namespace {

std::string ToLimitedString(const google::protobuf::Message& message, std::size_t max_size) {
    return utils::log::ToLimitedUtf8(message.Utf8DebugString(), max_size);
}

}  // namespace

std::string GetMessageForLogging(const google::protobuf::Message& message, MessageLoggingOptions options) {
    if (!logging::ShouldLog(options.log_level)) {
        return "hidden by log level";
    }

    if (!options.trim_secrets || !HasSecrets(message)) {
        return ToLimitedString(message, options.max_size);
    }

    std::unique_ptr<google::protobuf::Message> trimmed{message.New()};
    trimmed->CopyFrom(message);
    TrimSecrets(*trimmed);
    return ToLimitedString(*trimmed, options.max_size);
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
