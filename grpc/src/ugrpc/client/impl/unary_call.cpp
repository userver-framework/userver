#include <userver/ugrpc/client/impl/unary_call.hpp>

#include <userver/ugrpc/client/impl/tracing.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void AccountStatistics(ugrpc::impl::RpcStatisticsScope& stats, CompletionStatus completion_status) noexcept {
    if (completion_status.has_value()) {
        const auto& status = completion_status.value();
        stats.OnExplicitFinish(status.error_code());
    } else {
        switch (completion_status.error()) {
            case SpecialCaseCompletionType::kNetworkError:
                stats.OnNetworkError();
                break;
            case SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
                stats.OnCancelledByDeadlinePropagation();
                break;
            case SpecialCaseCompletionType::kCancelled:
                stats.OnCancelled();
                break;
            case SpecialCaseCompletionType::kAbandoned:
                // Nothing to do with statistics, `RpcStatisticsScope` automatically accounts "abandoned-error"
                break;
        }
    }
    stats.Flush();
}

void SetStatusForSpan(tracing::Span& span, CompletionStatus completion_status) noexcept {
    if (completion_status.has_value()) {
        const auto& status = completion_status.value();
        impl::SetStatusForSpan(span, status);
    } else {
        switch (completion_status.error()) {
            case SpecialCaseCompletionType::kNetworkError:
                impl::SetErrorForSpan(span, "Call interrupted, Network error");
                break;
            case SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
                impl::SetErrorForSpan(span, "Call cancelled by deadline propagation");
                break;
            case SpecialCaseCompletionType::kCancelled:
                impl::SetErrorForSpan(span, "Call cancelled");
                break;
            case SpecialCaseCompletionType::kAbandoned:
                impl::SetErrorForSpan(span, "Call abandoned");
                break;
        }
    }
}

[[noreturn]] void ThrowSpecialCaseCompletionError(
    std::string_view call_name,
    SpecialCaseCompletionType special_case_completion_type
) {
    switch (special_case_completion_type) {
        case SpecialCaseCompletionType::kNetworkError:
            throw RpcInterruptedError{call_name, "UnaryCall (Network error)"};
        case SpecialCaseCompletionType::kTimeoutDeadlinePropagated:
            throw RpcCancelledError{call_name, "UnaryCall (Deadline Propagation)"};
        case SpecialCaseCompletionType::kCancelled:
            throw RpcCancelledError{call_name, "UnaryCall"};
        case SpecialCaseCompletionType::kAbandoned:
            throw RpcCancelledError{call_name, "UnaryCall (Abandoned)"};
    }
    UINVARIANT(false, "unreachable");  // (gcc 11): ‘noreturn’ function does return
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
