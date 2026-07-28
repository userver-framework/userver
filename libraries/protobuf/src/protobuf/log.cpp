#include <userver/protobuf/log.hpp>

#include <userver/protobuf/json/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

logging::LogHelper& operator<<(logging::LogHelper& h, const google::protobuf::Message& message) {
    try {
        constexpr std::size_t kDefaultLoggingStringLimit = 1024;
        return h << protobuf::json::MessageToDebugString(message, kDefaultLoggingStringLimit);
    } catch (const std::exception& ex) {
        return h << "Failed to log message: " << ex;
    }
}

}  // namespace logging

USERVER_NAMESPACE_END
