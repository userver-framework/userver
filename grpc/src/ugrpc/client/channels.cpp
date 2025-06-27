#include <userver/ugrpc/client/channels.hpp>

#include <algorithm>

#include <grpcpp/create_channel.h>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>

#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

namespace impl {

namespace {

[[nodiscard]] bool
DoTryWaitForConnected(grpc::Channel& channel, grpc::CompletionQueue& queue, engine::Deadline deadline) {
    while (true) {
        // A potentially-blocking call
        const auto state = channel.GetState(true);

        if (state == ::GRPC_CHANNEL_READY) return true;
        if (state == ::GRPC_CHANNEL_SHUTDOWN) return false;

        ugrpc::impl::AsyncMethodInvocation operation;
        channel.NotifyOnStateChange(state, deadline, &queue, operation.GetCompletionTag());
        if (operation.Wait() != ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk) return false;
    }
}

}  // namespace

[[nodiscard]] bool TryWaitForConnected(
    ClientData& client_data,
    engine::Deadline deadline,
    engine::TaskProcessor& blocking_task_processor
) {
    const auto stub_state = client_data.GetStubState();
    const auto& channels = stub_state->stubs.GetChannels();

    auto& queue = client_data.NextQueue();

    std::vector<engine::TaskWithResult<bool>> tasks{};
    tasks.reserve(channels.size());
    for (auto& channel : channels) {
        tasks.emplace_back(engine::AsyncNoSpan(
            blocking_task_processor, DoTryWaitForConnected, std::ref(*channel), std::ref(queue), deadline
        ));
    }
    const auto results = engine::GetAll(tasks);
    return std::all_of(results.begin(), results.end(), [](bool connected) { return connected; });
}

}  // namespace impl

std::shared_ptr<grpc::Channel> MakeChannel(
    engine::TaskProcessor& blocking_task_processor,
    std::shared_ptr<grpc::ChannelCredentials> channel_credentials,
    const std::string& endpoint
) {
    // Spawn a blocking task creating a gRPC channel
    // This is third party code, no use of span inside it
    return engine::AsyncNoSpan(
               blocking_task_processor,
               grpc::CreateChannel,
               ugrpc::impl::ToGrpcString(endpoint),
               std::ref(channel_credentials)
    )
        .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
