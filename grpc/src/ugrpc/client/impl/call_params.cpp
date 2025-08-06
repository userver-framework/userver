#include <userver/ugrpc/client/impl/call_params.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/algo.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>

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

void ApplyDynamicConfig(CallOptions& call_options, const Qos& qos, const testsuite::GrpcControl& testsuite_grpc) {
    // dynamic config has lower priority, than set manually by user in CallOptions
    // so set only if NOT set

    if (std::chrono::milliseconds::max() == call_options.GetTimeout() && qos.timeout.has_value()) {
        call_options.SetTimeout(testsuite_grpc.MakeTimeout(*qos.timeout));
    }

    if (0 == call_options.GetAttempts() && qos.attempts.has_value()) {
        call_options.SetAttempts(*qos.attempts);
    }
}

void ApplyStaticConfig(CallOptions& call_options, const RetryConfig& retry_config) {
    // static config has lowest priority,
    // so set only if NOT set

    if (0 == call_options.GetAttempts()) {
        call_options.SetAttempts(retry_config.attempts);
    }
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
    ApplyDynamicConfig(call_options, qos, client_data.GetTestsuiteControl());
    ApplyStaticConfig(call_options, client_data.GetRetryConfig());

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
