#include <userver/ugrpc/client/completion_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

std::string_view ToString(SpecialCaseCompletionType type) {
    switch (type) {
        case SpecialCaseCompletionType::kNetworkError:
            return "network_error";
        case SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
            return "timeout_deadline_propagated";
        case SpecialCaseCompletionType::kCancelled:
            return "cancelled";
        case SpecialCaseCompletionType::kAbandoned:
            return "abandoned";
    }
    return "unknown";
}

std::string_view GetSpecialCaseCompletionTypeDescription(SpecialCaseCompletionType type) {
    switch (type) {
        case SpecialCaseCompletionType::kNetworkError:
            return "underlying network operation failed (e.g., connection lost, socket error)";
        case SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
            return "timed out because of the propagated deadline from the upstream request";
        case SpecialCaseCompletionType::kCancelled:
            return "task was cancelled (e.g. calling code dropped TaskWithResult object, or upstream client cancelled "
                   "the request)";
        case SpecialCaseCompletionType::kAbandoned:
            return "object representing the call was destroyed before the call could complete normally";
    }

    return "unknown completion type";
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
