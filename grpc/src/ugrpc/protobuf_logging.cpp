#include <userver/ugrpc/protobuf_logging.hpp>

#include <exception>
#include <string>
#include <string_view>

#include <userver/formats/json/string_builder.hpp>
#include <userver/protobuf/json/convert.hpp>

#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/status_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

constexpr std::string_view kTruncateMarker = "...(truncated)";

}  // namespace

std::string ToLimitedLoggingString(const google::protobuf::Message& message, std::size_t limit) noexcept {
    try {
        if (limit == 0) {
            return std::string{kTruncateMarker};
        }

        std::string result = protobuf::json::MessageToDebugString(message, limit);

        // MessageToDebugString stops only by overshooting the limit (closing braces / quotes always push a
        // truncated JSON strictly past `limit`), so a longer-than-`limit` result means the message was cut off.
        if (result.size() > limit) {
            return fmt::format("{}\n{}", result, kTruncateMarker);
        }

        return result;
    } catch (const std::exception& ex) {
        return fmt::format("serialization failed: {}", ex.what());
    }
}

std::string ToLimitedLoggingString(const grpc::Status& status, std::size_t limit) noexcept {
    formats::json::StringBuilder sb;
    {
        const formats::json::StringBuilder::ObjectGuard guard{sb};

        sb.Key("code");
        sb.WriteString(ugrpc::ToStringView(status.error_code()));

        if (!status.error_message().empty()) {
            sb.Key("message");
            sb.WriteString(status.error_message());
        }

        const auto gstatus = ugrpc::ToGoogleRpcStatus(status);
        if (gstatus.has_value()) {
            sb.Key("details");
            sb.WriteRawString(ugrpc::ToLimitedLoggingString(*gstatus, limit));
        }
    }

    return sb.GetString();
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
