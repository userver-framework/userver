#include <userver/ugrpc/client/impl/call_params.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/numeric_cast.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>

#include <ugrpc/client/impl/compat/retry_policy.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void CheckValidCallName(std::string_view call_name) {
    UASSERT_MSG(!call_name.empty(), "generic call_name must NOT be empty");
    UASSERT_MSG(call_name[0] != '/', utils::StrCat("generic call_name must NOT start with /, given: ", call_name));
    UASSERT_MSG(
        call_name.find('/') != std::string_view::npos,
        utils::StrCat("generic call_name must contain /, given: ", call_name)
    );
}

void SetAttempts(CallOptions& call_options, const Qos& qos, const RetryConfig& retry_config) {
    if (0 == call_options.GetAttempts()) {
        call_options.SetAttempts(qos.attempts.value_or(retry_config.attempts));
    }
}

void SetTimeout(CallOptions& call_options, const Qos& qos, const testsuite::GrpcControl& testsuite_grpc) {
    if (std::chrono::milliseconds::max() == call_options.GetTimeout() && qos.timeout.has_value()) {
        call_options.SetTimeout(testsuite_grpc.MakeTimeout(*qos.timeout));
    }
}

void SetTimeoutStreaming(
    CallOptions& call_options,
    const Qos& qos,
    const RetryConfig& retry_config,
    const testsuite::GrpcControl& testsuite_grpc
) {
    SetTimeout(call_options, qos, testsuite_grpc);

    // if timeout is set, reset it to TotalTimeout, because of grpc-core retries
    const auto timeout = call_options.GetTimeout();
    if (std::chrono::milliseconds::max() != timeout) {
        const auto attempts = qos.attempts.value_or(retry_config.attempts);
        UINVARIANT(0 < attempts, "Qos/RetryConfig attempts value must be greater than 0");
        const auto total_timeout = compat::CalculateTotalTimeout(timeout, utils::numeric_cast<std::uint32_t>(attempts));
        call_options.SetTimeout(total_timeout);
    }
}

void ApplyRetryConfiguration(
    CallOptions& call_options,
    const Qos& qos,
    const RetryConfig& retry_config,
    const testsuite::GrpcControl& testsuite_grpc
) {
    SetAttempts(call_options, qos, retry_config);
    SetTimeout(call_options, qos, testsuite_grpc);
}

void ApplyRetryConfigurationStreaming(
    CallOptions& call_options,
    const Qos& qos,
    const RetryConfig& retry_config,
    const testsuite::GrpcControl& testsuite_grpc
) {
    // we use grpc-core retries for streaming-methods,
    // so CallOption attempts do not work, no need to set them

    SetTimeoutStreaming(call_options, qos, retry_config, testsuite_grpc);
}

}  // namespace

CallParams CreateCallParams(const ClientData& client_data, std::size_t method_id, CallOptions&& call_options) {
    const auto& metadata = client_data.GetMetadata();
    const auto call_name = GetMethodFullName(metadata, method_id);

    if (engine::current_task::ShouldCancel()) {
        throw RpcCancelledError(call_name, "RPC construction");
    }

    auto stub = client_data.NextStubFromMethodId(method_id);

    const auto qos = stub.GetClientQos().methods.GetOptional(call_name).value_or(Qos{});
    ApplyRetryConfiguration(call_options, qos, client_data.GetRetryConfig(), client_data.GetTestsuiteControl());
    if (ugrpc::impl::RpcType::kUnary == GetMethodType(metadata, method_id)) {
        ApplyRetryConfiguration(call_options, qos, client_data.GetRetryConfig(), client_data.GetTestsuiteControl());
    } else {
        ApplyRetryConfigurationStreaming(
            call_options, qos, client_data.GetRetryConfig(), client_data.GetTestsuiteControl()
        );
    }

    return CallParams{
        client_data.GetClientName(),
        client_data.NextQueue(),
        client_data.GetConfigSnapshot(),
        {ugrpc::impl::MaybeOwnedString::Ref{}, call_name},
        std::move(call_options),
        std::move(stub),
        client_data.GetMiddlewares(),
        client_data.GetStatistics(method_id),
        client_data.GetTestsuiteControl(),
    };
}

CallParams CreateGenericCallParams(
    const ClientData& client_data,
    std::string_view call_name,
    CallOptions&& call_options,
    GenericOptions&& generic_options
) {
    CheckValidCallName(call_name);
    if (generic_options.metrics_call_name.has_value()) {
        CheckValidCallName(*generic_options.metrics_call_name);
    }

    if (engine::current_task::ShouldCancel()) {
        throw RpcCancelledError(call_name, "RPC construction");
    }

    UINVARIANT(!client_data.GetClientQos(), "Client QOS configs are unsupported for generic services");

    return CallParams{
        client_data.GetClientName(),
        client_data.NextQueue(),
        client_data.GetConfigSnapshot(),
        ugrpc::impl::MaybeOwnedString{std::string{call_name}},
        std::move(call_options),
        client_data.NextStub(),
        client_data.GetMiddlewares(),
        client_data.GetGenericStatistics(generic_options.metrics_call_name.value_or(call_name)),
        client_data.GetTestsuiteControl(),
    };
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
