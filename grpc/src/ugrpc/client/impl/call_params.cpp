#include <userver/ugrpc/client/impl/call_params.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/algo.hpp>

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

// gRPC is known to behave badly when dealing with huge deadlines.
//
// "timeout = 1 year ought to be enough for anybody"
//
// Still, sometimes people set timeouts of many years to mean infinity.
// We'll support that use case and consider such timeouts infinite.
constexpr std::chrono::hours kMaxSafeDeadline{24 * 365};

void SetTimeout(
    grpc::ClientContext& context,
    std::optional<std::chrono::milliseconds> timeout,
    const testsuite::GrpcControl& testsuite_control
) {
    if (timeout && *timeout <= kMaxSafeDeadline) {
        context.set_deadline(std::chrono::system_clock::now() + testsuite_control.MakeTimeout(*timeout));
    } else {
        context.set_deadline(std::chrono::system_clock::time_point::max());
    }
}

// Order of timeout application, from highest to lowest priority:
// 1. Qos passed as a parameter at the RPC creation
// 2. manual client_context manipulation by the user
// 3. GRPC_CLIENT_QOS dynamic config
void ApplyQosConfigs(
    const ClientData& client_data,
    grpc::ClientContext& client_context,
    const Qos& user_qos,
    std::string_view call_name
) {
    if (user_qos.timeout) {
        // Consider the explicit Qos parameter the highest-priority source.
        // TODO there is no way to override other sources by setting this timeout
        // to infinity (we treat it as "not set")
        SetTimeout(client_context, user_qos.timeout, client_data.GetTestsuiteControl());
        return;
    }

    if (client_context.deadline() != std::chrono::system_clock::time_point::max()) {
        // Deadline has already been set in client_context by the user. Consider it
        // a high-priority source.
        return;
    }

    if (const auto* const config_key = client_data.GetClientQos()) {
        const auto config = client_data.GetConfigSnapshot();
        if (const auto dynamic_qos = config[*config_key].GetOptional(call_name)) {
            SetTimeout(client_context, dynamic_qos->timeout, client_data.GetTestsuiteControl());
        }
    }
}

}  // namespace

CallParams CreateCallParams(
    const ClientData& client_data,
    std::size_t method_id,
    std::unique_ptr<grpc::ClientContext> client_context,
    const Qos& qos
) {
    const auto& metadata = client_data.GetMetadata();
    const auto call_name = metadata.method_full_names[method_id];

    if (engine::current_task::ShouldCancel()) {
        throw RpcCancelledError(call_name, "RPC construction");
    }

    ApplyQosConfigs(client_data, *client_context, qos, call_name);

    return CallParams{
        client_data.GetClientName(),  //
        client_data.NextQueue(),
        client_data.GetConfigSnapshot(),
        {ugrpc::impl::MaybeOwnedString::Ref{}, call_name},
        std::move(client_context),
        client_data.GetStatistics(method_id),
        client_data.GetMiddlewares(),
    };
}

CallParams CreateGenericCallParams(
    const ClientData& client_data,
    std::string_view call_name,
    std::unique_ptr<grpc::ClientContext> client_context,
    const Qos& qos,
    std::optional<std::string_view> metrics_call_name
) {
    CheckValidCallName(call_name);
    if (metrics_call_name) {
        CheckValidCallName(*metrics_call_name);
    }

    if (engine::current_task::ShouldCancel()) {
        throw RpcCancelledError(call_name, "RPC construction");
    }

    ApplyQosConfigs(client_data, *client_context, qos, call_name);

    return CallParams{
        client_data.GetClientName(),  //
        client_data.NextQueue(),
        client_data.GetConfigSnapshot(),
        ugrpc::impl::MaybeOwnedString{std::string{call_name}},
        std::move(client_context),
        client_data.GetGenericStatistics(metrics_call_name.value_or(call_name)),
        client_data.GetMiddlewares(),
    };
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
